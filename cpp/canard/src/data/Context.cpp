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
    if (m_functions.find(identifier) != m_functions.end())
        return false;
    m_functions.emplace(identifier, f);
    return true;
}

const FunctionRef &Context::get(const std::string &name) {
    auto it = m_functions.find(name);
    if (it != m_functions.end())
        return it->second;
    if (m_parent)
        return m_parent->get(name);
    return FunctionRef::null();
}
