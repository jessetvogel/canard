//
// Created by Jesse Vogel on 01/07/2021.
//

#include "namespace.h"

#include <utility>

Namespace::Namespace(Session &session) : m_session(session), m_parent(nullptr), m_name(std::string()), m_context() {}

Namespace::Namespace(Namespace &parent, std::string name) : m_session(parent.m_session),
                                                            m_parent(&parent),
                                                            m_name(std::move(name)), m_context() {}

FunctionPtr Namespace::get_function(std::string &path) {
    size_t i = path.find('.');
    if (i == std::string::npos)
        return m_context.get_function(path);

    // Otherwise, ask a child
    auto it = m_children.find(path.substr(0, i));
    if (it == m_children.end())
        return nullptr;

    std::string sub_path = path.substr(i + 1);
    return it->second->get_function(sub_path);
}

Namespace *Namespace::get_parent() {
    return m_parent;
}

Namespace *Namespace::get_namespace(std::string &path) {
    if (path.empty())
        return this;

    size_t i = path.find('.');
    if (i == std::string::npos) { // if path has no dots, return one of the children
        auto it = m_children.find(path);
        if (it == m_children.end())
            return nullptr;
        return it->second.get();
    }

    // Otherwise, ask a child
    auto it = m_children.find(path.substr(0, i));
    if (it == m_children.end())
        return nullptr;
    std::string sub_path = path.substr(i + 1);
    return it->second->get_namespace(sub_path);
}

Context &Namespace::get_context() {
    return m_context;
}

Namespace *Namespace::create_subspace(const std::string& name) {
    auto subspace = new Namespace(*this, name);
    std::unique_ptr<Namespace> child(subspace);
    m_children.emplace(name, std::move(child));
    return subspace;
}

std::string Namespace::to_string() {
    if (m_parent == nullptr)
        return m_name;

    std::string parent_str = m_parent->to_string();
    if (parent_str.empty())
        return m_name;

    return parent_str + '.' + m_name;
}
