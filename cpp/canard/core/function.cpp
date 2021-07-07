//
// Created by Jesse Vogel on 01/07/2021.
//

#include "function.h"
#include "matcher.h"
#include "specialization.h"

#include <utility>
#include <sstream>

Function::Function(FunctionPtr type, DependencyData dependencies) : m_type(std::move(type)),
                                                                    m_dependencies(std::move(dependencies)) {};

FunctionPtr Function::get_type() {
    // m_type == nullptr indicates that function is Type, it's type is itself
    // however, we don't store pointers to self, because then we never destruct
    return m_type != nullptr ? m_type : shared_from_this();
}

void Function::set_label(const std::string &label) {
    m_label = label;
}

void Function::set_namespace(Namespace *space) {
    m_space = space;
}

FunctionPtr Function::get_base() {
    return shared_from_this();
}

std::vector<FunctionPtr> Function::get_explicit_dependencies() {
    size_t n = m_dependencies.size();
    std::vector<FunctionPtr> explicit_dependencies;
    for (int i = 0; i < n; ++i) {
        if (m_dependencies.m_explicits[i])
            explicit_dependencies.push_back(m_dependencies.m_functions[i]);
    }
    return explicit_dependencies;
}

const std::vector<FunctionPtr> &Function::get_arguments() {
    return m_dependencies.m_functions;
}

bool Function::depends_on(const std::vector<FunctionPtr> &list) {
    return std::find_if(list.begin(), list.end(),
                        [this](FunctionPtr const &ptr) { return ptr.get() == this; }) != list.end();
}

bool Function::depends_on(const std::unordered_set<FunctionPtr> &set) {
    return std::find_if(set.begin(), set.end(),
                        [this](FunctionPtr const &ptr) { return ptr.get() == this; }) != set.end();
}

bool Function::signature_depends_on(const std::vector<FunctionPtr> &list) {
    if (get_type()->depends_on(list))
        return true;
//    return std::any_of(m_dependencies.m_functions.begin(), m_dependencies.m_functions.end(),
//                       [list](auto &it) { return it->signature_depends_on(list); });
    for (auto &it : m_dependencies.m_functions) {
        if (it->signature_depends_on(list))
            return true;
    }
    return false;
}

FunctionPtr Function::specialize(std::vector<FunctionPtr> arguments, DependencyData dependencies) {
    // Check that the number of arguments equals the number of explicit dependencies
    size_t n = arguments.size();
    std::vector<FunctionPtr> explicit_dependencies = get_explicit_dependencies();
    size_t m = explicit_dependencies.size();
    if (n != m)
        throw SpecializationException(to_string() + " expected " + std::to_string(m) + " arguments but received " + std::to_string(n));

    // Base case: if there are no arguments and no given dependencies, immediately return
    if (n == 0 && dependencies.empty())
        return shared_from_this();

    // Create a matcher that matches the dependencies to the arguments given
    Matcher matcher(m_dependencies.m_functions);
    for (int i = 0; i < n; ++i) {
        FunctionPtr &dependency = explicit_dependencies[i];
        FunctionPtr &argument = arguments[i];
        if (!matcher.matches(dependency, argument))
            throw SpecializationException(
                    "argument '" + argument->to_string() + "' does not match '" + dependency->to_string() + "'");
    }

    // If this is a specialization itself (that is, the base is not equal to this),
    // we convert the arguments to arguments for the base, and then return a specialization of the base.
    // Note this must be done recursively, since specializing the base requires a different Matcher
    // Note also that base must be converted as well! E.g. when specializing 'def dom {T : Type} {X Y : T} (f : Morphism X Y) := X'
    FunctionPtr base = get_base();
    if (base.get() != this) {
        std::vector<FunctionPtr> base_arguments;
        for (auto &f : get_arguments())
            base_arguments.push_back(matcher.convert(f));
        return matcher.convert(base)->specialize(base_arguments, std::move(dependencies));
    }

    // TODO: if the list of arguments equals the list of dependencies, we can just return the underlying base function right ??

    // Now we deal with the case where this is a base function:
    // At this point everything is verified.
    // The last thing to do is to construct the type of the specialization.
    FunctionPtr converted_type = matcher.convert(get_type());

    // Now create a new specialization with the right inputs
    return std::make_shared<Specialization>(shared_from_this(), arguments, converted_type, std::move(dependencies));
}

std::string Function::to_string() {
    return to_string(false, false);
}

std::string Function::to_string(bool full, bool with_namespaces) {
    std::ostringstream ss;

    if (with_namespaces && m_space != nullptr) {
        std::string path = m_space->to_string();
        if (!path.empty())
            ss << path << '.';
    }

    ss << (m_label.empty() ? "?" : m_label);

//    ss << "[" << (((size_t) this) % 10000) << "]";

    if (full) {
        size_t n = m_dependencies.m_functions.size();
        for (int i = 0; i < n; ++i) {
            bool e = m_dependencies.m_explicits[i];
            ss << (e ? " (" : " {");
            ss << m_dependencies.m_functions[i]->to_string(true, with_namespaces);
            ss << (e ? ')' : '}');
        }
        ss << " : ";
        ss << get_type()->to_string(false, with_namespaces);
    }

    return ss.str();
}

bool Function::equals(const FunctionPtr &other) {
    if (other->get_base() != get_base())
        return false;

    // Must have the same number of dependencies
    size_t n = m_dependencies.m_functions.size();
    if (n != other->m_dependencies.m_functions.size())
        return false;

    // If there are no dependencies, check equality on the arguments
    if (n == 0) {
        auto &arguments = get_arguments(), &other_arguments = other->get_arguments();
        size_t m = arguments.size();
        for (int i = 0; i < m; ++i)
            if (!arguments[i]->equals(other_arguments[i]))
                return false;
        return true;
    }

    // If there are dependencies, we need a Matcher
    Matcher matcher(m_dependencies.m_functions);
    for (int i = 0; i < n; i++) {
        // Explicitness must match
        if (m_dependencies.m_explicits[i] != other->m_dependencies.m_explicits[i])
            return false;
        if (!matcher.matches(m_dependencies.m_functions[i], m_dependencies.m_functions[i]))
            return false;
    }

    // Now just let the matcher do its work
    return matcher.matches(shared_from_this(), other);
}