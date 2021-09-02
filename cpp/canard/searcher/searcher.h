//
// Created by Jesse Vogel on 01/07/2021.
//

#pragma once

#include "query.h"
#include <queue>

class Searcher {

    const int m_max_depth;

    std::queue<std::shared_ptr<Query>> m_queue;
    std::vector<FunctionPtr> m_all_theorems, m_generic_theorems;
    std::unordered_map<FunctionPtr, std::vector<FunctionPtr>> m_index;

    std::vector<FunctionPtr> m_result;

    bool search_helper(std::shared_ptr<Query> &, FunctionPtr &, std::vector<std::shared_ptr<Query>> &);

    bool is_redundant(const std::shared_ptr<Query> &, const std::shared_ptr<Query> &);

public:

    explicit Searcher(int);

    void add_namespace(Namespace &);

    bool search(std::shared_ptr<Query> &);

    std::vector<FunctionPtr> get_result() { return m_result; }

};
