//
// Created by Jesse Vogel on 01/07/2021.
//

#include "Function.h"
#include "Matcher.h"
#include <utility>
#include <memory>

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

Function::Function(const FunctionRef &type, Telescope parameters) {
    m_type = type;
    m_parameters = std::move(parameters);
    m_base = nullptr;
}

Function::Function(const FunctionRef &type, Telescope parameters, const FunctionRef &base, std::vector<FunctionRef> arguments) {
    m_type = type;
    m_parameters = std::move(parameters);
    m_base = base;
    m_arguments = std::move(arguments);
}

FunctionRef Function::make(const FunctionRef &type, Telescope parameters) {
    return std::shared_ptr<Function>(new Function(type, std::move(parameters)));
}

FunctionRef Function::make(const FunctionRef &type, Telescope parameters, const FunctionRef &base, std::vector<FunctionRef> arguments) {
    return std::shared_ptr<Function>(new Function(type, std::move(parameters), base, std::move(arguments)));
}

const std::string &Function::name() const {
    return m_name;
}

const Telescope &Function::parameters() const {
    return m_parameters;
}

const std::vector<FunctionRef> &Function::arguments() const {
    // If the Function is a base-function, its arguments are its own parameters
    return is_base() ? m_parameters.functions() : m_arguments;
}

void Function::set_name(const std::string &name) {
    m_name = name;
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
        return false;
    }
}

bool Function::depends_on(const std::unordered_set<FunctionRef> &set) {
    if (is_base()) {
        // A base function is said to 'depend' on the set, if it is contained in the set
        for (const auto &ptr: set) {
            if (ptr.operator->() == this)
                return true;
        }
        return false;
    } else {
        // A specialization is said to 'depend' on the set, if one of its arguments depends on the set
        for (auto &arg: m_arguments) {
            if (arg->depends_on(set))
                return true;
        }
        return false;
    }
}

bool Function::signature_depends_on(const std::vector<FunctionRef> &list) {
    if (m_type == nullptr ? depends_on(list) : m_type->depends_on(list)) return true;
//    return std::any_of(m_parameters.m_functions.begin(), m_parameters.m_functions.end(),
//                       [list](auto &it) { return it->signature_depends_on(list); });
    for (const auto &param: m_parameters.functions()) {
        if (param->signature_depends_on(list))
            return true;
    }
    return false;
}

void Function::set_metadata(std::shared_ptr<void> metadata) {
    m_metadata = std::move(metadata);
}

FunctionRef FunctionRef::specialize(Telescope parameters, std::vector<FunctionRef> arguments) const {
    // Check that the number of arguments equals the number of parameters
    const size_t m = m_f->m_parameters.size();
    const size_t n = arguments.size();
    if (n != m)
        throw SpecializationException( // TODO: change SpecializationException so that formatting can be done on the level where the exception is catched!
                "Formatter::to_string(*this) +  expected " + std::to_string(m) +
                " arguments but received " + std::to_string(n));

    // Base case: if there are no arguments and no given parameters, immediately return
    if (n == 0 && parameters.empty())
        return *this;

    // Create a matcher that matches the dependencies to the arguments given
    Matcher matcher(m_f->parameters().functions());
    for (int i = 0; i < n; ++i) {
        const FunctionRef &parameter = m_f->parameters().functions()[i];
        const FunctionRef &argument = arguments[i];
        if (argument == nullptr) // `nullptr` indicates an implicit argument
            continue;
        if (!matcher.matches(parameter, argument))
            throw SpecializationException(
                    "argument ' + Formatter::to_string(argument) + ' does not match ' + Formatter::to_string(parameter) + '");
    }

    // Swap any `nullptr` arguments with their match
    for (int i = 0; i < n; ++i) {
        if (arguments[i] == nullptr) {
            const auto &solution = matcher.get_solution(m_f->parameters().functions()[i]);
            if (solution == nullptr)
                throw SpecializationException("Could not deduce implicit argument " + std::to_string(i));
            arguments[i] = solution;
        }
    }

    // TODO: this must change at some point !!
    // If this is a specialization itself,
    // we convert the arguments to arguments for the base, and then return a specialization of the base.
    // Note this must be done recursively, since specializing the base requires a different Matcher
    // Note also that base must be converted as well! E.g. when specializing 'def dom {T : Type} {X Y : T} (f : Morphism X Y) := X'
    if (!m_f->is_base()) {
        std::vector<FunctionRef> base_arguments;
        for (auto &f: m_f->arguments())
            base_arguments.push_back(matcher.convert(f));
        return matcher.convert(base()).specialize(std::move(parameters), std::move(base_arguments));
    }

    // TODO: if the list of arguments equals the list of dependencies, we can just return the underlying base function right ??

    // Now we deal with the case where this is a base function:
    // At this point everything is verified.
    // The last thing to do is to construct the type of the specialization.
    // Now create a new specialization with the right inputs
    return Function::make(matcher.convert(type()), std::move(parameters), *this, std::move(arguments));
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
