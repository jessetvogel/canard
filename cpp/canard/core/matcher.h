//
// Created by Jesse Vogel on 01/07/2021.
//

#pragma once

#include <vector>
#include "function.h"

class Matcher {

    Matcher* m_parent;
    const std::vector<FunctionPtr>& m_indeterminates;
    std::unordered_map<FunctionPtr, FunctionPtr> m_solutions;

    std::vector<FunctionPtr>* m_locals = nullptr;
    std::unordered_map<FunctionPtr, std::shared_ptr<std::unordered_set<FunctionPtr>>>* m_locals_allowed = nullptr;

    bool put_solution(const FunctionPtr&, const FunctionPtr&);

    bool is_indeterminate(const FunctionPtr&);

public:

    explicit Matcher(const std::vector<FunctionPtr>&);
    explicit Matcher(Matcher*, const std::vector<FunctionPtr>&);

    void set_locals(std::vector<FunctionPtr>*, std::unordered_map<FunctionPtr, std::shared_ptr<std::unordered_set<FunctionPtr>>>*);

    bool matches(const FunctionPtr&, const FunctionPtr&);
    void assert_matches(const FunctionPtr&, const FunctionPtr&);

    FunctionPtr get_solution(const FunctionPtr&) const;
    FunctionPtr convert(const FunctionPtr&);
    FunctionPtr clone(const FunctionPtr&);
    FunctionPtr cheap_clone(const FunctionPtr&);

    std::string to_string();

};
