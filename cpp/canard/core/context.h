//
// Created by Jesse Vogel on 01/07/2021.
//

#pragma once

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include "function.h"

class Context {

    Context *const m_parent;
    std::vector<FunctionPtr> m_all_functions;
    std::unordered_map<std::string, FunctionPtr> m_labels;
    std::unordered_set<FunctionPtr> m_used_functions;

public:

    Context();

    Context(Context&);

    bool put_function(const std::string&, const FunctionPtr&);

    FunctionPtr get_function(const std::string &);

    bool is_used(const FunctionPtr &);

    std::vector<FunctionPtr>& get_functions();

};
