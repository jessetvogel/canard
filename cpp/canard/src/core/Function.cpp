//
// Created by Jesse Vogel on 01/07/2021.
//

#include "Function.h"
#include "Matcher.h"
#include <utility>
#include <memory>

const FunctionRef &FunctionRef::null() {
    static FunctionRef null = nullptr;
    return null;
}

FunctionRef::FunctionRef(std::shared_ptr<Function> ptr) {
    m_f = std::move(ptr);
}

FunctionRef::FunctionRef(const FunctionRef &other) {
    m_f = other.m_f;
}

const FunctionRef &FunctionRef::type() const {
    // If m_type == nullptr, then function is Type, whose type is itself
    // This is because we don't store pointers to self, because then we never destruct
    return m_f->m_type == nullptr ? *this : m_f->m_type;
}

const FunctionRef &FunctionRef::base() const {
    // If m_base == nullptr, then function is a base function, whose base is itself
    // This is because we don't store pointers to self, because then we never destruct
    return m_f->m_base == nullptr ? *this : m_f->m_base;
}

Function::Function(Telescope parameters, const FunctionRef &type) {
    m_parameters = std::move(parameters);
    m_type = type;
    m_base = nullptr;
}

Function::Function(Telescope parameters, const FunctionRef &type, const FunctionRef &base, std::vector<FunctionRef> arguments) {
    m_parameters = std::move(parameters);
    m_type = type;
    m_base = base;
    m_arguments = std::move(arguments);
}

FunctionRef Function::make(Telescope parameters, const FunctionRef &type) {
    return std::shared_ptr<Function>(new Function(std::move(parameters), type));
}

FunctionRef Function::make(Telescope parameters, const FunctionRef &type, const FunctionRef &base, std::vector<FunctionRef> arguments) {
    return std::shared_ptr<Function>(new Function(std::move(parameters), type, base, std::move(arguments)));
}

const std::vector<FunctionRef> &Function::arguments() const {
    // If the Function is a base-function, its arguments are its own parameters
    return is_base() ? m_parameters.functions() : m_arguments;
}

void Function::set_name(const std::string &name) {
    m_name = name;
}

void Function::set_implicit(bool implicit) {
    m_implicit = implicit;
}

void Function::set_constructor(const FunctionRef &constructor) {
    m_constructor = constructor;
}

void Function::set_space(void *space) {
    m_space = space;
}

bool Function::is_constructor() const {
    return (m_type != nullptr) && (m_type.base()->constructor().operator->() == this);
}

bool FunctionRef::depends_on(const std::vector<FunctionRef> &list) const {
    if (m_f->is_base()) {
        // A base function is said to 'depend' on the list, if it is contained in the list
        return std::find(list.begin(), list.end(), *this) != list.end();
    } else {
        // A specialization is said to 'depend' on the list, if one of its arguments or the base depends on the list
        for (const auto &g: m_f->arguments()) {
            if (g.depends_on(list))
                return true;
        }
        return base().depends_on(list);
    }
}

bool FunctionRef::signature_depends_on(const std::vector<FunctionRef> &list) const {
    // Signature depends on list if the type or one of the parameters depends on the list
    if (type().depends_on(list))
        return true;
    for (const auto &g: m_f->parameters().functions()) {
        if (g.signature_depends_on(list))
            return true;
    }
    return false;
}

FunctionRef FunctionRef::specialize(const Telescope &parameters, std::vector<FunctionRef> arguments) const {
    // Check that the number of arguments equals the number of parameters
    const auto &f_parameter_functions = m_f->parameters().functions();
    const size_t m = f_parameter_functions.size();
    const size_t n = arguments.size();
    if (n > m)
        throw SpecializationException("{f} expected " + std::to_string(m) + " arguments but received " + std::to_string(n), {{"f", *this}});

    // Base case: if there are no arguments and no given parameters, immediately return
    if (n == 0 && parameters.empty())
        return *this;

    // Create a matcher that matches the (first n) parameters to the (n) arguments given
    Matcher matcher(f_parameter_functions);
    for (int i = 0; i < n; ++i) {
        const FunctionRef &parameter = f_parameter_functions[i];
        const FunctionRef &argument = arguments[i];
        if (argument == nullptr) // `nullptr` indicates an implicit argument
            continue;
        if (!matcher.matches(parameter, argument))
            throw SpecializationException("argument {f} does not match {g}",
                                          {{"f", argument},
                                           {"g", parameter}});
    }

    // Swap any `nullptr` arguments with their match
    for (int i = 0; i < n; ++i) {
        if (arguments[i] == nullptr) {
            const auto &solution = matcher.get_solution(f_parameter_functions[i]);
            if (solution == nullptr)
                throw SpecializationException("could not infer implicit argument for parameter {f}", {{"f", f_parameter_functions[i]}});
            arguments[i] = solution;
        }
    }

    // If less arguments were given than f has parameters, clone the excess parameters of f
    // and add them both to the parameters of the specialization and the arguments
    Telescope parameters_full = parameters;
    for (int i = (int) n; i < m; ++i) {
        auto clone = matcher.clone(f_parameter_functions[i]);
        matcher.assert_matches(f_parameter_functions[i], clone);
        parameters_full.add(clone);
        arguments.push_back(clone);
    }

    // If this is a specialization itself, we sort_and_convert the arguments to arguments for the base, and then return a specialization of the base.
    // This must be done recursively, as specializing the base requires a different Matcher.
    // Note that base must be converted as well!
    if (!m_f->is_base())
        return matcher.convert(base()).specialize(parameters_full, matcher.convert(m_f->arguments()));

    // Specialize constructor if needed
    auto constructor = (m_f->constructor()) ? m_f->constructor().specialize(parameters, arguments) : nullptr;

    // Create a new specialization with the right inputs
    auto f = Function::make(
            std::move(parameters_full), matcher.convert(type()),
            *this, std::move(arguments)
    );

    // Set metadata
    f->set_implicit(m_f->implicit());
    f->set_constructor(constructor);

    return f;
}

bool FunctionRef::equivalent(const FunctionRef &other) const {
    // Shortcut
    if (m_f == other.m_f) return true;

    // Bases should agree
    if (other.base() != base()) return false;

    // Must have the same number of parameters
    const size_t n = m_f->m_parameters.functions().size();
    if (n != other->m_parameters.functions().size()) return false;

    // If there are no parameters, check equality on the arguments
    if (n == 0) {
        const auto &arguments = m_f->arguments(), &other_arguments = other->arguments();
        const size_t m = arguments.size();
        for (int i = 0; i < m; ++i)
            if (!arguments[i].equivalent(other_arguments[i]))
                return false;
        return true;
    }

    // If there are dependencies, we need a Matcher
    Matcher matcher(m_f->m_parameters.functions());
    for (int i = 0; i < n; i++) {
        if (!matcher.matches(m_f->m_parameters.functions()[i], other->m_parameters.functions()[i]))
            return false;
    }

    // Now just let the matcher do its work
    return matcher.matches(*this, other);
}

FunctionRef &FunctionRef::operator=(FunctionRef other) {
    std::swap(m_f, other.m_f);
    return *this;
}
