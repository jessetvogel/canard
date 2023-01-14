//
// Created by Jesse Vogel on 01/07/2021.
//

#include "Context.h"

#include <utility>

Context::Context() : m_parent(nullptr), m_is_space(true), m_name(std::string()), m_full_name(std::string()) {}

Context::Context(Context &parent) : m_parent(&parent), m_name(std::string()), m_is_space(false), m_full_name(std::string()) {}

Context::Context(Context &parent, std::string name) : m_parent(&parent),
                                                      m_is_space(true),
                                                      m_name(std::move(name)),
                                                      m_full_name(parent.full_name().empty() ? m_name : parent.full_name() + '.' + m_name) {}

bool Context::put(const std::string &path, const FunctionRef &f, bool only_reference) {
    const size_t i = path.find('.');
    if (i != std::string::npos) {
        const std::string prefix = path.substr(0, i);
        const std::string suffix = path.substr(i + 1);
        return get_subspace(prefix).put(suffix, f, only_reference);
    }

    if (m_functions.find(path) != m_functions.end())
        return false;

    m_functions.emplace(path, f);
    if (!only_reference) {
        f->set_name(path);
        if (is_space())
            f->set_space(this);
    }
    return true;
}

const FunctionRef &Context::get(const std::string &path) const {
    const size_t i = path.find('.');
    if (i != std::string::npos) {
        const std::string prefix = path.substr(0, i);
        const std::string suffix = path.substr(i + 1);
        auto sub_context = find_subspace(prefix);
        return (sub_context == nullptr) ? FunctionRef::null() : sub_context->get(suffix);
    }

    auto it = m_functions.find(path);
    if (it != m_functions.end())
        return it->second;
    return FunctionRef::null();
}

Context *Context::find_subspace(const std::string &path) {
    if (path.empty())
        return this;

    const size_t i = path.find('.');
    if (i != std::string::npos) {
        const std::string prefix = path.substr(0, i);
        const std::string suffix = path.substr(i + 1);
        auto it = m_subspaces.find(prefix);
        return (it == m_subspaces.end()) ? nullptr : it->second->find_subspace(suffix);
    }

    auto it = m_subspaces.find(path);
    return (it == m_subspaces.end()) ? nullptr : it->second.get();
}

const Context *Context::find_subspace(const std::string &path) const {
    if (path.empty())
        return this;

    const size_t i = path.find('.');
    if (i != std::string::npos) {
        const std::string prefix = path.substr(0, i);
        const std::string suffix = path.substr(i + 1);
        auto it = m_subspaces.find(prefix);
        return (it == m_subspaces.end()) ? nullptr : it->second->find_subspace(suffix);
    }

    auto it = m_subspaces.find(path);
    return (it == m_subspaces.end()) ? nullptr : it->second.get();
}

Context &Context::get_subspace(const std::string &path) {
    if (path.empty())
        return *this;

    const size_t i = path.find('.');
    if (i != std::string::npos) {
        const std::string prefix = path.substr(0, i);
        const std::string suffix = path.substr(i + 1);
        auto it = m_subspaces.find(prefix);
        auto child = (it != m_subspaces.end()) ? it->second.get() : create_subspace(prefix);
        return child->get_subspace(suffix);
    }

    auto it = m_subspaces.find(path);
    return (it != m_subspaces.end()) ? *it->second : *create_subspace(path);
}

Context *Context::create_subspace(const std::string &name) {
    auto subspace = new Context(*this, name);
    std::unique_ptr<Context> child(subspace);
    m_subspaces.emplace(name, std::move(child));
    return subspace;
}

void Context::set_preference(const FunctionRef &f, int x) {
    m_preferences[f] = x;
}

int Context::get_preference(const FunctionRef &f) {
    auto it = m_preferences.find(f);
    if (it == m_preferences.end())
        return DEFAULT_PREFERENCE;
    return it->second;
}
