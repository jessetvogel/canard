//
// Created by Jesse Vogel on 03/08/2022.
//

#pragma once

#include <string>
#include "../core/Telescope.h"

class Structure {
public:

private:

    const std::string name;
    Telescope m_parameters;
    std::vector<FunctionRef> m_fields;

};


