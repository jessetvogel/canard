//
// Created by Jesse Vogel on 01/07/2021.
//

#include "session.h"

#include <memory>

Session::Session() {
    TYPE = std::make_shared<Function>(nullptr, DependencyData());
    PROP = std::make_shared<Function>(TYPE, DependencyData());

    m_global_namespace = std::make_unique<Namespace>(*this);
    m_global_namespace->get_context().put_function("Type", TYPE);
    m_global_namespace->get_context().put_function("Prop", PROP);
}

Namespace &Session::get_global_namespace() {
    return *m_global_namespace;
}
