//
// Created by Jesse Vogel on 01/07/2021.
//

#pragma once

#include "Context.h"

class Session {
private:

    std::unique_ptr<Context> m_global_namespace;

public:

    Session();

    FunctionRef TYPE = nullptr;
    FunctionRef PROP = nullptr;

    Context &global_namespace() { return *m_global_namespace; };

};
