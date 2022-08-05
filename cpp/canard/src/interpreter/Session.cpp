//
// Created by Jesse Vogel on 01/07/2021.
//

#include <memory>
#include "Session.h"
#include "Namespace.h"

Session::Session() {
    TYPE = Function::make(nullptr, Telescope{});
    PROP = Function::make(TYPE, Telescope{});

    m_global_namespace = std::unique_ptr<Namespace>(new Namespace());
    m_global_namespace->get_context().put_function("Type", TYPE);
    m_global_namespace->get_context().put_function("Prop", PROP);
}

Namespace &Session::get_global_namespace() {
    return *m_global_namespace;
}
