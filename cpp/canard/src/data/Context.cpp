//
// Created by Jesse Vogel on 01/07/2021.
//

#include "Context.h"

Context::Context() : m_parent(nullptr) {}

Context::Context(Context &parent) : m_parent(&parent) {}

bool Context::put(const FunctionRef &f) {
    return put(f->name(), f);
}

bool Context::put(const std::string &identifier, const FunctionRef &f) {
    if (m_map.find(identifier) != m_map.end())
        return false;
    m_map.emplace(identifier, f);
    return true;
}

const FunctionRef &Context::get(const std::string &name) const {
    auto it = m_map.find(name);
    if (it != m_map.end())
        return it->second;
    if (m_parent)
        return m_parent->get(name);
    return FunctionRef::null();
}
