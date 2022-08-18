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

bool Function::depends_on(const std::vector<FunctionRef> &list) {
    if (is_base()) {
        // A base function is said to 'depend' on the list, if it is contained in the list
        for (const auto &ptr: list) {
            if (ptr.operator->() == this)
                return true;
        }
        return false;
    } else {
        // A specialization is said to 'depend' on the list, if one of its arguments depends on the list
        for (auto &arg: m_arguments) {
            if (arg->depends_on(list))
                return true;
        }
        // TODO: what about the base ?
        return false;
    }
}

bool Function::signature_depends_on(const std::vector<FunctionRef> &list) {
    if (m_type == nullptr ? depends_on(list) : m_type->depends_on(list)) return true;
//    return std::any_of(m_parameters.m_map.begin(), m_parameters.m_map.end(),
//                       [list](auto &it) { return it->signature_depends_on(list); });
    for (const auto &param: m_parameters.functions()) {
        if (param->signature_depends_on(list))
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
        throw SpecializationException(*this, nullptr,
                                      "{f} expected " + std::to_string(m) + " arguments but received " + std::to_string(n));

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
            throw SpecializationException(argument, parameter, "argument {f} does not match {g}");
    }

    // Swap any `nullptr` arguments with their match
    for (int i = 0; i < n; ++i) {
        if (arguments[i] == nullptr) {
            const auto &solution = matcher.get_solution(f_parameter_functions[i]);
            if (solution == nullptr)
                throw SpecializationException(nullptr, nullptr,
                                              "could not infer implicit argument for parameter '" + f_parameter_functions[i]->name() + "'");
            arguments[i] = solution;
        }
    }

    // TODO: if f is base and parameters and arguments are precisely equal, we might want to return the base itself!

    // If less arguments were given than f has parameters, clone the excess parameters of f
    // and add them both to the parameters of the specialization and the arguments
    Telescope parameters_full = parameters;
    for (int i = (int) n; i < m; ++i) {
        auto clone = matcher.clone(f_parameter_functions[i]);
        // TODO: implicitness must also be copied!
        matcher.assert_matches(f_parameter_functions[i], clone);
        parameters_full.add(clone);
        arguments.push_back(clone);
    }

    // TODO: this must(?) change at some point !
    // If this is a specialization itself,
    // we convert the arguments to arguments for the base, and then return a specialization of the base.
    // Note this must be done recursively, since specializing the base requires a different Matcher
    // Note also that base must be converted as well! E.g. when specializing 'def dom {T : Type} {X Y : T} (f : Morphism X Y) := X'
    if (!m_f->is_base())
        return matcher.convert(base()).specialize(parameters_full, matcher.convert(m_f->arguments()));

    // Specialize constructor if needed
    auto constructor = (m_f->constructor())
                       ? m_f->constructor().specialize(parameters, arguments)
                       : nullptr;
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
