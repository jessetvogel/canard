//
// Created by Jesse Vogel on 01/07/2021.
//

#pragma once

#include "../core/function.h"
#include "../core/matcher.h"
#include <memory>

class Query {

    const std::shared_ptr<Query> m_parent;
    const std::vector<FunctionPtr> m_indeterminates;
    std::vector<int> m_depths;
    std::unordered_map<FunctionPtr, FunctionPtr> m_solutions;
    std::vector<FunctionPtr> m_locals;
    std::unordered_map<FunctionPtr, std::shared_ptr<std::unordered_set<FunctionPtr>>> m_locals_allowed; // TODO: does the unordered_set NEED to be shared_ptr ?? Can we not just omit it, and use std::move where possible ?

    Query(std::shared_ptr<Query>, std::unordered_map<FunctionPtr, FunctionPtr>, std::vector<FunctionPtr>,
          std::vector<int>);

    bool injects_into_helper(Matcher *, std::vector<FunctionPtr>, std::vector<FunctionPtr>);

    bool is_allowed_solution(const FunctionPtr &, const FunctionPtr &);

    bool is_allowed_solution(const std::unordered_set<FunctionPtr> &, const FunctionPtr &);

public:

    explicit Query(std::vector<FunctionPtr>);

    inline const std::shared_ptr<Query> &get_parent() { return m_parent; };

    inline const std::vector<FunctionPtr> &get_locals() { return m_locals; }

    const FunctionPtr &get_last_indeterminate();
    int get_depth();

    size_t get_indeterminates_size() { return m_indeterminates.size(); }

    inline bool is_solved() { return m_indeterminates.empty(); }

    bool is_indeterminate(const FunctionPtr &);
    static std::shared_ptr<Query> normalize(const std::shared_ptr<Query> &);
    static std::shared_ptr<Query> reduce(const std::shared_ptr<Query> &, const FunctionPtr &);
    static std::vector<FunctionPtr> get_final_solutions(const std::shared_ptr<Query> &);
    bool injects_into(const std::shared_ptr<Query> &);
    std::string to_string();

//    int hash_code();

};
