//
// Created by Jesse Vogel on 01/07/2021.
//

#pragma once

#include "Query.h"
#include "ThreadManager.h"
#include "Index.h"
#include "../data/Context.h"
#include <queue>
#include <mutex>
#include <set>
#include <atomic>

struct QueryEntry {

    std::shared_ptr<Query> query;
    std::vector<int> order;

    bool operator<(const QueryEntry &other) const {
        if (query->complexity() != other.query->complexity())
            return query->complexity() > other.query->complexity();
        return order > other.order; // compare lexicographically
    }
};

class Searcher {
public:

    Searcher(const std::unordered_set<Context *> &, int max_depth, int max_threads = 1);

    bool search(const Telescope &, int max_results = 1);
    bool prove(const FunctionRef &);
    void clear();

    Index &index() { return m_index; }
    const std::vector<std::vector<FunctionRef>> &results() const { return m_results; }

    int query_counter() const { return m_query_counter; }

private:

    enum SearchResult {
        SEARCH_CONTINUE, // continue searching
        SEARCH_STOP, // stop searching for this query
        SEARCH_DONE // stop searching at all
    };

    const int m_max_depth;
    std::atomic<bool> m_searching;
    int m_max_results = 0;
    int m_result_counter = 0;
    int m_query_counter = 0;

    ThreadManager m_thread_manager;
    std::mutex m_mutex;
    std::priority_queue<QueryEntry, std::vector<QueryEntry>> m_queue;
    std::vector<std::vector<FunctionRef>> m_results;
    FunctionRef m_excluded_thm; // used for `prove()`
    Index m_index;

    void search_loop();
    SearchResult search_helper(const std::shared_ptr<Query> &, const FunctionRef &, std::vector<std::shared_ptr<Query>> &);
    bool check_reasonable(const std::shared_ptr<Query> &q, const std::shared_ptr<Query> &p);
    bool check_checkpoints(const std::shared_ptr<Query> &);

    bool is_easier_than(const Query &, const Query &);

};
