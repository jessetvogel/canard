//
// Created by Jesse Vogel on 01/07/2021.
//

#pragma once

#include "Query.h"
#include "ThreadManager.h"
#include "../data/Namespace.h"
#include <queue>
#include <mutex>

class Searcher {

public:

    explicit Searcher(int max_depth, int max_threads = 1);

    void add_namespace(Namespace &);
    bool search(const std::shared_ptr<Query> &);
//    void optimize(const std::shared_ptr<Query> &);

    const std::vector<FunctionRef> &result() const { return m_result; }

private:

    const int m_max_depth;
    bool m_searching = false;

    ThreadManager m_thread_manager;
    std::mutex m_mutex;
    std::vector<std::queue<std::shared_ptr<Query>>> m_depth_queues;
    std::vector<FunctionRef> m_all_theorems, m_generic_theorems;
    std::unordered_map<FunctionRef, std::vector<FunctionRef>> m_index;
    std::vector<FunctionRef> m_result;

    void search_loop();
    bool search_helper(std::shared_ptr<Query> &, const FunctionRef &, std::vector<std::shared_ptr<Query>> &);
    bool is_redundant(const std::shared_ptr<Query> &, const std::shared_ptr<Query> &);

};
