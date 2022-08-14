//
// Created by Jesse Vogel on 01/07/2021.
//

#include <memory>
#include "Session.h"
#include "Namespace.h"
#include "Metadata.h"

Session::Session() {
    m_global_namespace = std::unique_ptr<Namespace>(new Namespace());

    TYPE = Function::make({}, nullptr);
    PROP = Function::make({}, TYPE);

    TYPE->set_name("Type");
    PROP->set_name("Prop");

    TYPE->set_metadata(std::make_shared<Metadata>());
    PROP->set_metadata(std::make_shared<Metadata>());

    m_global_namespace->context().put(TYPE);
    m_global_namespace->context().put(PROP);
}
