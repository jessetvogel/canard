//
// Created by Jesse Vogel on 01/07/2021.
//

#pragma once

#include "Query.h"
#include "ThreadManager.h"
#include "../data/Namespace.h"
#include <queue>
#include <mutex>

struct QueryEntry {
    std::shared_ptr<Query> query;
    int fifo_order;
};

class CompareQuery {
public:
    bool operator()(const QueryEntry &entry_1, const QueryEntry &entry_2) {
        const int cost1 = entry_1.query->cost();
        const int cost2 = entry_2.query->cost();
        if (cost1 == cost2)
            return entry_1.fifo_order > entry_2.fifo_order;
        return cost1 > cost2;
    }
};

class Searcher {
public:

    explicit Searcher(int max_depth, int max_threads = 1);

    void add_namespace(Namespace &);
    bool search(const Telescope &);
    bool prove(const FunctionRef &);
    void clear();

    const std::vector<FunctionRef> &result() const { return m_result; }

private:

    const int m_max_depth;
    bool m_searching = false;
    int fifo_counter = 0;

    ThreadManager m_thread_manager;
    std::mutex m_mutex;
    std::priority_queue<QueryEntry, std::vector<QueryEntry>, CompareQuery> m_queue;
    std::vector<FunctionRef> m_all_theorems, m_generic_theorems;
    std::unordered_map<FunctionRef, std::vector<FunctionRef>> m_index;
    std::vector<FunctionRef> m_result;
    FunctionRef m_excluded_thm;

    void search_loop();
    bool search_helper(std::shared_ptr<Query> &, const FunctionRef &, std::vector<std::shared_ptr<Query>> &);
    bool is_redundant(const std::shared_ptr<Query> &, const std::shared_ptr<Query> &);

};
