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
    const std::vector<int> &depths() const { return m_depths; }
    const std::vector<int> &locals_depths() const { return m_locals_depths; }
    const std::unordered_map<FunctionRef, FunctionRef> &solutions() const { return m_solutions; }
    const FunctionRef &goal(int * = nullptr) const;
    int depth() const { return m_depth; };
    int complexity() const { return m_complexity; };

    bool is_solved() const { return goal() == nullptr; }
    std::vector<FunctionRef> final_solutions() const; // TODO: maybe rename this to `backtrack_solutions` or `compute_solutions` or something

    const Query *checkpoint() const { return m_checkpoint; }
    int distance_to_checkpoint() const;
    bool set_checkpoint(const Query &);

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
    const std::vector<std::vector<FunctionRef>> m_locals;
    const std::vector<int> m_locals_depths;
    const std::unordered_map<FunctionRef, FunctionRef> m_solutions;
    const int m_depth;
    const int m_complexity;

    const Query *m_checkpoint = nullptr;

    bool is_allowed_solution(int, const FunctionRef &);

    int compute_depth() const;
    int compute_complexity() const;

};
