//
// Created by Jesse Vogel on 01/07/2021.
//

#pragma once

#include "../core/Function.h"
#include "../core/Matcher.h"
#include <memory>

class Query {

    const std::shared_ptr<Query> m_parent;
    const std::vector<FunctionRef> m_indeterminates;
    std::vector<int> m_depths;
    std::unordered_map<FunctionRef, FunctionRef> m_solutions;
    std::vector<FunctionRef> m_locals;
    std::unordered_map<FunctionRef, std::shared_ptr<std::unordered_set<FunctionRef>>> m_locals_allowed; // TODO: does the unordered_set NEED to be shared_ptr ?? Can we not just omit it, and use std::move where possible ?

    Query(std::shared_ptr<Query>, std::unordered_map<FunctionRef, FunctionRef>, std::vector<FunctionRef>,
          std::vector<int>);

    bool injects_into_helper(Matcher *, std::vector<FunctionRef>, std::vector<FunctionRef>);
    bool is_allowed_solution(const FunctionRef &, const FunctionRef &);
    bool is_allowed_solution(const std::unordered_set<FunctionRef> &, const FunctionRef &);

public:

    explicit Query(std::vector<FunctionRef>);

    const std::shared_ptr<Query> &parent() const { return m_parent; }
    const std::vector<FunctionRef> &locals() const { return m_locals; }
    const FunctionRef &last_indeterminate() const { return m_indeterminates.back(); }

    int depth();

    size_t indeterminates_size() { return m_indeterminates.size(); }

    bool is_solved() { return m_indeterminates.empty(); }
//    bool has_parent(const std::shared_ptr<Query>&) const;

    bool is_indeterminate(const FunctionRef &);
    static std::shared_ptr<Query> normalize(const std::shared_ptr<Query> &);
    static std::shared_ptr<Query> reduce(const std::shared_ptr<Query> &, const FunctionRef &);
    static std::vector<FunctionRef> final_solutions(const std::shared_ptr<Query> &);
    bool injects_into(const std::shared_ptr<Query> &);
    std::string to_string();

//    int hash_code();

};
