//
// Created by Jesse Vogel on 01/07/2021.
//

#include "Namespace.h"
#include "Context.h"

Context::Context() : m_parent(nullptr) {}

Context::Context(Context &parent) : m_parent(&parent) {}

bool Context::put_function(const std::string &label, const FunctionPtr &f) {
    auto pos = m_labels.find(label);
    if (pos != m_labels.end())
        return false;

    f->set_label(label);
    m_labels.emplace(label, f);
    return true;
}

FunctionPtr Context::get_function(const std::string &label) {
    auto it = m_labels.find(label);
    if (it != m_labels.end()) {
        m_used_functions.insert(it->second);
        return it->second;
    }

    if (m_parent != nullptr)
        return m_parent->get_function(label);

    return nullptr;
}

bool Context::is_used(const FunctionPtr &f) {
    return m_used_functions.find(f) != m_used_functions.end();
}
