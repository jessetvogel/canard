//
// Created by Jesse Vogel on 25/10/2021.
//

#pragma once

#include "../core/Formatter.h"

class EnglishFormatter : public Formatter {
public:

    virtual std::string format(const FunctionPtr&);

};


