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
    static std::shared_ptr<Query> reduce(const std::shared_ptr<Query> &, const FunctionRef &); // TODO: why is this static ?

    std::vector<FunctionRef> final_solutions();

    explicit Query(Telescope);

    const std::shared_ptr<Query> &parent() const { return m_parent; }
    const Telescope &telescope() const { return m_telescope; }
    const FunctionRef &goal() const { return telescope().functions().back(); }
    const std::vector<int> &depths() const { return m_depths; }
    const std::vector<int> &context_depths() const { return m_context_depths; }
    const std::unordered_map<FunctionRef, FunctionRef> &solutions() const { return m_solutions; }
    Context &context() const { return *m_context; }

    int depth() const { return m_depth; };

    bool is_solved() { return m_telescope.empty(); }
    bool injects_into(const std::shared_ptr<Query> &);

private:

    Query(std::shared_ptr<Query>, Telescope, std::vector<int>, std::vector<int>, std::unordered_map<FunctionRef, FunctionRef>);

    const std::shared_ptr<Query> m_parent;
    const Telescope m_telescope;
    const std::vector<int> m_depths;
    const std::vector<int> m_context_depths;
    const int m_depth = 0;
    const std::unordered_map<FunctionRef, FunctionRef> m_solutions;

    std::unique_ptr<Context> m_own_context;
    Context *m_context;
    int m_context_depth = 0;

    void create_own_context();
    void escape_to_context_depth(int);

    bool injects_into_helper(Matcher *, std::vector<FunctionRef>, std::vector<FunctionRef>);
    bool is_allowed_solution(int, const FunctionRef &);

};
