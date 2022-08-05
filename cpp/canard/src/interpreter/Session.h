//
// Created by Jesse Vogel on 01/07/2021.
//

#pragma once

#include "Namespace.h"
#include "../core/Function.h"

class Session {
private:

//    FunctionPool m_pool; // (Note that we put this in the top of the header-file so that it gets destructed last)
    std::unique_ptr<Namespace> m_global_namespace;

public:

    Session();

    FunctionRef TYPE = nullptr;
    FunctionRef PROP = nullptr;
    
    Namespace &get_global_namespace();

};
