//
// Created by Jesse Vogel on 01/07/2021.
//

#pragma once

#include "../core/Function.h"
#include "../core/Matcher.h"
#include "../data/Context.h"
#include <memory>

class Query {
public:

    static std::shared_ptr<Query> normalize(const std::shared_ptr<Query> &);
    static std::shared_ptr<Query> reduce(const std::shared_ptr<Query> &, const FunctionRef &);

    explicit Query(Telescope);

    const std::shared_ptr<Query> &parent() const { return m_parent; }
    const Telescope &telescope() const { return m_telescope; }
    const std::vector<std::vector<FunctionRef>> &locals() const { return m_locals; }
    const FunctionRef &goal() const { return m_telescope.empty() ? FunctionRef::null() : m_telescope.functions().back(); }
    const std::vector<int> &depths() const { return m_depths; }
    const std::vector<int> &locals_depths() const { return m_locals_depths; }
    const std::unordered_map<FunctionRef, FunctionRef> &solutions() const { return m_solutions; }

    int depth() const { return m_depth; };

    bool is_solved() const { return m_telescope.empty(); }
    bool injects_into(const std::shared_ptr<Query> &);

    std::vector<FunctionRef> final_solutions() const;

private:

    Query(std::shared_ptr<Query> query,
          Telescope telescope,
          std::vector<int> depths,
          std::vector<std::vector<FunctionRef>> locals,
          std::vector<int> locals_depths,
          std::unordered_map<FunctionRef, FunctionRef> solutions);

    const std::shared_ptr<Query> m_parent;
    const Telescope m_telescope;
    const std::vector<int> m_depths;
    const int m_depth = 0;
    const std::vector<std::vector<FunctionRef>> m_locals;
    const std::vector<int> m_locals_depths;
    const std::unordered_map<FunctionRef, FunctionRef> m_solutions;

    bool injects_into_helper(Matcher *, std::vector<FunctionRef>, std::vector<FunctionRef>);
    bool is_allowed_solution(int, const FunctionRef &);

};
