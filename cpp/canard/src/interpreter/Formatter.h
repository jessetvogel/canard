//
// Created by Jesse Vogel on 25/10/2021.
//

#pragma once

#include "../core/Function.h"

class Formatter {
public:

    static std::string to_string(const FunctionRef &);
    static std::string to_string(const FunctionRef &, bool, bool);

};
