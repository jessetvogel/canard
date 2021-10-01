//
// Created by Jesse Vogel on 01/07/2021.
//

#pragma once

#include "Namespace.h"
#include "Function.h"
#include "Specialization.h"

class Session {

    std::unique_ptr<Namespace> m_global_namespace;

public:

    Session();

    Namespace& get_global_namespace();

    FunctionPtr TYPE, PROP;
};
