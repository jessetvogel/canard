//
// Created by Jesse Vogel on 01/07/2021.
//

#include <memory>
#include "Session.h"

Session::Session() {
    m_global_namespace = std::unique_ptr<Context>(new Context());

    TYPE = Function::make({}, nullptr);
    PROP = Function::make({}, TYPE);
    
    m_global_namespace->put("Type", TYPE);
    m_global_namespace->put("Prop", PROP);
}
