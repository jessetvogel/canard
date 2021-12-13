//
// Created by Jesse Vogel on 01/07/2021.
//

#include "Function.h"
#include "Matcher.h"
#include "Formatter.h"
#include "FunctionPool.h"
#include <utility>

void Function::init() {
    m_space = nullptr;
    m_label.clear();
    m_type = nullptr;
    m_base = nullptr;
    m_parameters = {};
    m_arguments.clear();
    m_reference_count = 0;
}

void Function::init(const FunctionPtr &type, FunctionParameters parameters) {
    m_space = nullptr;
    m_label.clear();
    m_type = type;
    m_base = nullptr;
    m_parameters = std::move(parameters);
    m_arguments.clear();
    m_reference_count = 0;
}

void Function::init(const FunctionPtr &type, FunctionParameters parameters,
                    const FunctionPtr &base, std::vector<FunctionPtr> arguments) {
    m_space = nullptr;
    m_label.clear();
    m_type = type;
    m_base = base;
    m_parameters = std::move(parameters);
    m_arguments = std::move(arguments);
    m_reference_count = 0;
}

FunctionPtr::FunctionPtr(FunctionPool *pool, Function *f) {
    m_pool = pool;
    m_f = f;
    if (m_f != nullptr) m_f->m_reference_count++;
}

FunctionPtr::FunctionPtr(const FunctionPtr &other) {
    m_pool = other.m_pool;
    m_f = other.m_f;
    if (m_f != nullptr) m_f->m_reference_count++;
}

FunctionPtr &FunctionPtr::operator=(FunctionPtr other) {
    std::swap(m_pool, other.m_pool);
    std::swap(m_f, other.m_f);
    return *this;
}

FunctionPtr::~FunctionPtr() {
    // If this was the last reference to the Function, recycle it
    if (m_f != nullptr) {
        if (--(m_f->m_reference_count) == 0) {
            m_f->init();
            m_pool->recycle(m_f);
        }
    }
}

const FunctionPtr &FunctionPtr::type() const {
    // m_type == nullptr indicates that function is Type, it's type is itself
    // However, we don't store pointers to self, because then we never destruct
    if (m_f->m_type == nullptr) return *this;
    return m_f->m_type;
}

const FunctionPtr &FunctionPtr::base() const {
    // If the Function is a base-function, it is its own base
    if (m_f->is_base()) return *this;
    return m_f->m_base;
}

const FunctionParameters &Function::parameters() const {
    return m_parameters;
}

const std::vector<FunctionPtr> &Function::arguments() const {
    // If the Function is a base-function, its arguments are its own parameters
    if (is_base()) return m_parameters.m_functions;
    // Otherwise, interpret its own functions as the arguments
    return m_arguments;
}

std::vector<FunctionPtr> Function::explicit_parameters() const {
    std::vector<FunctionPtr> explicit_dependencies;
    const auto n = m_parameters.size();
    for (int i = 0; i < n; ++i) {
        if (m_parameters.m_explicits[i])
            explicit_dependencies.push_back(m_parameters.m_functions[i]);
    }
    return explicit_dependencies;
}

const std::string &Function::label() const {
    return m_label;
}

Namespace *Function::space() const {
    return m_space;
}

void Function::set_label(const std::string &label) {
    m_label = label;
}

void Function::set_namespace(Namespace *space) {
    m_space = space;
}

bool Function::depends_on(const std::vector<FunctionPtr> &list) {
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

bool Function::depends_on(const std::unordered_set<FunctionPtr> &set) {
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

bool Function::signature_depends_on(const std::vector<FunctionPtr> &list) {
    if (m_type == nullptr ? depends_on(list) : m_type->depends_on(list)) return true;
//    return std::any_of(m_parameters.m_functions.begin(), m_parameters.m_functions.end(),
//                       [list](auto &it) { return it->signature_depends_on(list); });
    for (const auto &param: m_parameters.m_functions) {
        if (param->signature_depends_on(list))
            return true;
    }
    return false;
}

FunctionPtr FunctionPtr::specialize(std::vector<FunctionPtr> arguments, FunctionParameters parameters) {
    // Check that the number of arguments equals the number of explicit dependencies
    std::vector<FunctionPtr> explicit_dependencies = m_f->explicit_parameters();
    const size_t m = explicit_dependencies.size();
    const size_t n = arguments.size();
    if (n != m)
        throw SpecializationException(
                Formatter::to_string(*this) + " expected " + std::to_string(m) +
                " arguments but received " + std::to_string(n));

    // Base case: if there are no arguments and no given dependencies, immediately return
    if (n == 0 && parameters.empty()) return *this;

    // Create a matcher that matches the dependencies to the arguments given
    Matcher matcher(m_f->m_parameters.m_functions);
    for (int i = 0; i < n; ++i) {
        FunctionPtr &dependency = explicit_dependencies[i];
        FunctionPtr &argument = arguments[i];
        if (!matcher.matches(dependency, argument))
            throw SpecializationException(
                    "argument '" + Formatter::to_string(argument) + "' does not match '" +
                    Formatter::to_string(dependency) + "'");
    }

    // If this is a specialization itself,
    // we convert the arguments to arguments for the base, and then return a specialization of the base.
    // Note this must be done recursively, since specializing the base requires a different Matcher
    // Note also that base must be converted as well! E.g. when specializing 'def dom {T : Type} {X Y : T} (f : Morphism X Y) := X'
    if (!m_f->is_base()) {
        std::vector<FunctionPtr> base_arguments;
        for (auto &f: m_f->arguments())
            base_arguments.push_back(matcher.convert(f));
        return matcher.convert(base()).specialize(std::move(base_arguments), std::move(parameters));
    }

    // TODO: if the list of arguments equals the list of dependencies, we can just return the underlying base function right ??

    // Now we deal with the case where this is a base function:
    // At this point everything is verified.
    // The last thing to do is to construct the type of the specialization.
    // Now create a new specialization with the right inputs
    return m_pool->create_specialization(matcher.convert(type()), std::move(parameters),
                                         *this, std::move(arguments));
}

bool FunctionPtr::equivalent(const FunctionPtr &other) const {
    // Shortcut
    if(m_f == other.m_f) return true;

    // Bases should agree
    if (other.base() != base()) return false;

    // Must have the same number of parameters
    const size_t n = m_f->m_parameters.m_functions.size();
    if (n != other->m_parameters.m_functions.size()) return false;

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
    Matcher matcher(m_f->m_parameters.m_functions);
    for (int i = 0; i < n; i++) {
        // Explicitness must match
        if (m_f->m_parameters.m_explicits[i] != other->m_parameters.m_explicits[i])
            return false;
        if (!matcher.matches(m_f->m_parameters.m_functions[i], m_f->m_parameters.m_functions[i]))
            return false;
    }

    // Now just let the matcher do its work
    return matcher.matches(*this, other);
}
