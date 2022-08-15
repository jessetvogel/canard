//
// Created by Jesse Vogel on 01/07/2021.
//

#pragma once

#include <vector>
#include <unordered_map>
#include "Function.h"

class Matcher {

public:

    static Matcher& dummy();

    explicit Matcher(const std::vector<FunctionRef> &);
    Matcher(Matcher *, const std::vector<FunctionRef> &);

    const std::vector<FunctionRef> &indeterminates() const { return m_indeterminates; }

    bool matches(const FunctionRef &, const FunctionRef &);
    void assert_matches(const FunctionRef &, const FunctionRef &);

    const FunctionRef &get_solution(const FunctionRef &) const;
    bool has_solution(const FunctionRef &);
    FunctionRef convert(const FunctionRef &);
    std::vector<FunctionRef> convert(const std::vector<FunctionRef> &);
    FunctionRef clone(const FunctionRef &);
    FunctionRef clone(const Telescope &, const FunctionRef &);
    Telescope clone(const Telescope &, const Telescope &, std::unique_ptr<Matcher> * = nullptr);
    FunctionRef cheap_clone(const FunctionRef &);

    bool solved();

private:

    Matcher *const m_parent;
    const std::vector<FunctionRef> &m_indeterminates;
    std::unordered_map<FunctionRef, FunctionRef> m_solutions;

    bool put_solution(const FunctionRef &, const FunctionRef &);
    bool is_indeterminate(const FunctionRef &);

};
