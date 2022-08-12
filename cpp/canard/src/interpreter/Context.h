//
// Created by Jesse Vogel on 01/07/2021.
//

#pragma once

#include <unordered_map>
#include "../core/Function.h"

class Context {
public:

    Context();
    Context(Context &);

    const std::unordered_map<std::string, FunctionRef> &functions() const { return m_functions; }

    bool put(const FunctionRef &);
    bool put(const std::string &, const FunctionRef &);
    const FunctionRef &get(const std::string &);

private:
    
    Context *const m_parent;
    std::unordered_map<std::string, FunctionRef> m_functions;

};
