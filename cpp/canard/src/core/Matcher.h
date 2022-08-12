//
// Created by Jesse Vogel on 01/07/2021.
//

#pragma once

#include <vector>
#include <unordered_map>
#include "Function.h"

class Matcher {

public:

    explicit Matcher(const std::vector<FunctionRef> &);
    Matcher(Matcher *, const std::vector<FunctionRef> &);

    bool matches(const FunctionRef &, const FunctionRef &);
    void assert_matches(const FunctionRef &, const FunctionRef &);

    const FunctionRef &get_solution(const FunctionRef &) const;
    FunctionRef convert(const FunctionRef &);
    FunctionRef clone(const FunctionRef &);
    FunctionRef cheap_clone(const FunctionRef &);

    bool solved();

private:
    
    Matcher *const m_parent;
    const std::vector<FunctionRef> &m_indeterminates;
    std::unordered_map<FunctionRef, FunctionRef> m_solutions;

    bool put_solution(const FunctionRef &, const FunctionRef &);
    bool is_indeterminate(const FunctionRef &);

};
