//
// Created by Jesse Vogel on 25/10/2021.
//

#pragma once

#include "Function.h"

class Formatter {
public:

    static std::string to_string(const FunctionPtr&);
    static std::string to_string(const FunctionPtr&, bool, bool);

    virtual std::string format(const FunctionPtr&) = 0;

};

