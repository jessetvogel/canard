//
// Created by Jesse Vogel on 01/07/2021.
//

#include "Namespace.h"
#include "Metadata.h"
#include "../core/macros.h"
#include <utility>

Namespace::Namespace() : m_name(std::string()) {}

Namespace::Namespace(Namespace &parent, std::string name) : m_parent(&parent),
                                                            m_name(std::move(name)), m_context() {}

FunctionRef Namespace::get_function(const std::string &path) {
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

void Namespace::put_function(const FunctionRef &f) {
    m_functions.push_back(f);
    auto metadata = (Metadata *) f->metadata();
    if (metadata == nullptr)
        CANARD_ERROR("Function has no metadata!");
    else
        metadata->m_space = this;
}

Namespace *Namespace::get_parent() {
    return m_parent;
}

Namespace *Namespace::get_namespace(const std::string &path) {
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

Namespace *Namespace::create_subspace(const std::string &name) {
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

std::vector<Namespace *> Namespace::get_children() {
    std::vector<Namespace *> children;
    children.reserve(m_children.size());
    for (auto &entry: m_children)
        children.push_back(entry.second.get());
    return children;
}
