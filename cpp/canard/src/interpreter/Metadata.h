//
// Created by Jesse Vogel on 03/08/2022.
//

#pragma once

#include "Namespace.h"

struct Metadata {

    Namespace *m_space = nullptr;

    bool m_implicit = false;

    FunctionRef m_constructor = nullptr;

};
