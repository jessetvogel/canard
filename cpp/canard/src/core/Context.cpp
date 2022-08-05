//
// Created by Jesse Vogel on 01/07/2021.
//

#include "Context.h"

Context::Context() : m_parent(nullptr) {}

Context::Context(Context &parent) : m_parent(&parent) {}

bool Context::put_function(const std::string &name, const FunctionRef &f) {
    auto pos = m_functions.find(name);
    if (pos != m_functions.end())
        return false;

    f->set_name(name);
    m_functions.emplace(name, f);
    return true;
}

FunctionRef Context::get_function(const std::string &label) {
    auto it = m_functions.find(label);
    if (it != m_functions.end()) {
//        m_used_functions.insert(it->second);
        return it->second;
    }

    if (m_parent != nullptr)
        return m_parent->get_function(label);

    return nullptr;
}
