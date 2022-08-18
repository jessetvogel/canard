//
// Created by Jesse Vogel on 01/07/2021.
//

#include "Namespace.h"
#include <utility>

Namespace::Namespace() : m_name(std::string()), m_full_name(m_name) {}

Namespace::Namespace(Namespace &parent, std::string name) : m_parent(&parent),
                                                            m_name(std::move(name)),
                                                            m_context(),
                                                            m_full_name(parent.full_name().empty() ? m_name : parent.full_name() + '.' + m_name) {}

Namespace *Namespace::get_namespace(const std::string &path) {
    if (path.empty())
        return this;

    size_t i = path.find('.');
    if (i == std::string::npos) { // if path contains no dots, return one of the children
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

Namespace *Namespace::create_subspace(const std::string &name) {
    auto subspace = new Namespace(*this, name);
    std::unique_ptr<Namespace> child(subspace);
    m_children.emplace(name, std::move(child));
    return subspace;
}

const FunctionRef &Namespace::get_function(const std::string &path) {
    // First try context
    const auto &f = m_context.get(path);
    if (f != nullptr)
        return f;

    // If path has a dot, can try to go to a child namespace
    const size_t i = path.find('.');
    if (i == std::string::npos)
        return FunctionRef::null();

    auto it = m_children.find(path.substr(0, i));
    if (it == m_children.end())
        return FunctionRef::null();

    return it->second->get_function(path.substr(i + 1));
}
