//
// Created by Jesse Vogel on 01/07/2021.
//

#include "Session.h"
#include <memory>

Session::Session() {
    TYPE = m_pool.create_Type();
    PROP = m_pool.create_function(TYPE, {});

    m_global_namespace = std::unique_ptr<Namespace>(new Namespace(*this));
    m_global_namespace->get_context().put_function("Type", TYPE);
    m_global_namespace->get_context().put_function("Prop", PROP);
}

Namespace &Session::get_global_namespace() {
    return *m_global_namespace;
}
