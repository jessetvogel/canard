//
// Created by Jesse Vogel on 01/07/2021.
//

#pragma once

#include "Namespace.h"
#include "../core/Function.h"

class Session {
private:

    std::unique_ptr<Namespace> m_global_namespace;

public:

    Session();

    FunctionRef TYPE = nullptr;
    FunctionRef PROP = nullptr;

    Namespace &global_namespace() { return *m_global_namespace; };

};
