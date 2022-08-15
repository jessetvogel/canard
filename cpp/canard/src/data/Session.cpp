//
// Created by Jesse Vogel on 01/07/2021.
//

#include <memory>
#include "Session.h"
#include "Namespace.h"

Session::Session() {
    m_global_namespace = std::unique_ptr<Namespace>(new Namespace());

    TYPE = Function::make({}, nullptr);
    PROP = Function::make({}, TYPE);

    TYPE->set_name("Type");
    PROP->set_name("Prop");

    m_global_namespace->context().put(TYPE);
    m_global_namespace->context().put(PROP);
}
