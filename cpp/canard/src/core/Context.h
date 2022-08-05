//
// Created by Jesse Vogel on 01/07/2021.
//

#pragma once

#include <unordered_map>
#include "Function.h"

class Context {
public:

    Context();
    Context(Context &);

    bool put_function(const std::string &, const FunctionRef &);
    FunctionRef get_function(const std::string &);

private:

    Context *const m_parent;
    std::unordered_map<std::string, FunctionRef> m_functions;

};
