//
// Created by Jesse Vogel on 01/07/2021.
//

#pragma once

#include "namespace.h"
#include "function.h"
#include "specialization.h"

class Session {

    std::unique_ptr<Namespace> m_global_namespace;

public:

    Session();

    Namespace& get_global_namespace();

    FunctionPtr TYPE, PROP;
};
