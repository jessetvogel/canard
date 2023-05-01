//
// Created by Jesse Vogel on 01/07/2021.
//

#include "Searcher.h"
#include "../core/macros.h"
#include "../parser/Formatter.h"
#include <algorithm>

Searcher::Searcher(const std::unordered_set<Context *> &spaces,
                   const int max_depth,
                   const int max_threads) : m_max_depth(max_depth),
                                            m_index(spaces),
                                            m_thread_manager(max_threads),
                                            m_searching(false) {}

bool Searcher::search(const Telescope &telescope, int max_results) {
    // Clear searcher
    clear();
    // Place initial query
    m_queue.push({std::make_shared<Query>(telescope), {}});
    // Reset counter
    m_query_counter = 1;
    // Set max_results
    m_max_results = max_results;
    if (max_results == 0)
        return true;
    // Create a pool of threads
    m_searching = true;
    m_thread_manager.start(&Searcher::search_loop, this);
    // Wait for all threads to end, and return
    m_thread_manager.join_all();
    return !m_results.empty();
}

bool Searcher::prove(const FunctionRef &f) {
    CANARD_ASSERT(f->is_base(), "prove only works on base functions");
    // Call `search` after marking f as excluded
    m_excluded_thm = f;
    bool success = search(Telescope({f}));
    m_excluded_thm = nullptr;
    return success;
}

void Searcher::search_loop() {
    // TODO: can we get rid of `goto`'s ?
    while (m_searching) {
        // Take the first query from the first (non-empty) queue, i.e. the one with the lowest depth
        std::shared_ptr<Query> query = nullptr;
        std::vector<int> order;
        m_mutex.lock();
        if (!m_queue.empty()) {
            auto &entry = m_queue.top();
            query = entry.query;
            order.reserve(entry.order.size() + 1); // already reserve for an extra entry
            order.insert(order.end(), entry.order.begin(), entry.order.end());
            m_queue.pop();
        }
        m_mutex.unlock();

        // If there is no query, we should wait for other threads to come with updates
        // Depending on the result of the update, we should continue or stop
        if (query == nullptr) {
            if (m_thread_manager.wait_for_update())
                continue;
            else
                break;
        }

        // Normalize the query before reducing: convert parameters of telescope to local variables
        query = Query::normalize(query);

        // Check for redundancies
        if (!check_reasonable(query, query->parent()))
            continue;

        // Check for checkpoints
        if (!check_checkpoints(query))
            continue;

        // Now we are going to look for reductions. Before pushing to the queue, we will store all reductions
        // in a list `reductions`. Then we will sort this list based on the number of telescope, which is
        // some form of optimization
        const auto &h_type_base = query->goal().type().base();
        std::vector<std::shared_ptr<Query>> reductions;

        // First list to search through is the list of local variables of query
        for (const auto &local_layer: query->locals()) {
            for (const auto &thm: local_layer) {
                switch (search_helper(query, thm, reductions)) {
                    case SEARCH_CONTINUE:
                        continue;
                    case SEARCH_STOP:
                        goto end_theorems;
                    case SEARCH_DONE:
                        goto end_while;
                }
            }
        }

        // If the type base is an indeterminate of query, there is nothing better to do then to try all theorems
        if (query->telescope().contains(h_type_base)) {
            for (auto &thm: m_index.all_theorems()) {
                switch (search_helper(query, thm, reductions)) {
                    case SEARCH_CONTINUE:
                        continue;
                    case SEARCH_STOP:
                        goto end_theorems;
                    case SEARCH_DONE:
                        goto end_while;
                }
            }
        } else {
            // If the type base is known to the index, try the corresponding list of theorems
            auto theorems = m_index.theorems(h_type_base);
            if (theorems != nullptr) {
                for (auto &thm: *theorems) {
                    switch (search_helper(query, thm, reductions)) {
                        case SEARCH_CONTINUE:
                            continue;
                        case SEARCH_STOP:
                            goto end_theorems;
                        case SEARCH_DONE:
                            goto end_while;
                    }
                }
            }
            // Also try the general theorems
            for (auto &thm: m_index.generic_theorems()) {
                switch (search_helper(query, thm, reductions)) {
                    case SEARCH_CONTINUE:
                        continue;
                    case SEARCH_STOP:
                        goto end_theorems;
                    case SEARCH_DONE:
                        goto end_while;
                }
            }
        }

        end_theorems:

        // Then add them to the queue with increasing order
        m_mutex.lock();
        order.push_back(0);
        for (auto &r: reductions) {
            m_queue.push({std::move(r), order});
            ++order.back();
            ++m_query_counter;
        }
        m_mutex.unlock();

        // Notify the other threads for updates
        m_thread_manager.send_update(true);
    }
    end_while:

    // Send a permanent update
    m_thread_manager.send_permanent_update(true);
    // When we are out of the loop, this boolean makes the other threads terminate as well
    m_searching = false;
}

Searcher::SearchResult
Searcher::search_helper(const std::shared_ptr<Query> &query, const FunctionRef &thm, std::vector<std::shared_ptr<Query>> &reductions) {
    // Shortcut: if not searching anymore, just return false and be done with it
    if (!m_searching)
        return SEARCH_DONE;

    // If thm is excluded, return false
    if (m_excluded_thm != nullptr && thm.depends_on({m_excluded_thm}))
        return SEARCH_CONTINUE;

    // Try reducing query using thm
    auto sub_query = Query::reduce(query, thm);
    if (sub_query == nullptr)
        return SEARCH_CONTINUE;

    // If the sub_query is completely is_solved (i.e. no more telescope) we have a new result!
    // Append it to the vector of results, and continue if we want more results, and be done otherwise
    if (sub_query->is_solved()) {
        m_mutex.lock();
        if (m_searching) {
            m_results.push_back(sub_query->final_solutions());
            if (++m_result_counter == m_max_results)
                m_searching = false;
        }
        m_mutex.unlock();
        return (m_searching ? SEARCH_CONTINUE : SEARCH_DONE);
    }

    // If the maximum depth was reached, omit this sub_query
    if (sub_query->depth() >= m_max_depth)
        return SEARCH_CONTINUE;

    // If `sub_query` is easier than its parent, make it the only reduction
    // Only do this if we are searching for a single solution
    if (m_max_results == 1 && is_easier_than(*sub_query, *query)) {
        reductions = {sub_query};
        return SEARCH_STOP;
    }

    // Add sub_query to queue
    reductions.push_back(std::move(sub_query));
    return SEARCH_CONTINUE;
}

void Searcher::clear() {
    m_queue = {};
    m_results.clear();
    m_query_counter = 0;
    m_result_counter = 0;
}

bool Searcher::check_reasonable(const std::shared_ptr<Query> &q, const std::shared_ptr<Query> &p) {
    // A query q is reasonable w.r.t. p if p (nor any of its parents) is not easier than q.
    if (p == nullptr) return true;
    if (p->checkpoint()) return true; // TODO: only need to check up to the nearest checkpoint !
    if (is_easier_than(*p, *q)) return false;
    return check_reasonable(q, p->parent());
}

bool Searcher::check_checkpoints(const std::shared_ptr<Query> &query) {
    // Find the nearest parent with a checkpoint
    int distance_to_checkpoint = 0;
    for (const Query *p = query->parent().get(); p != nullptr; p = p->parent().get()) {
        ++distance_to_checkpoint;
        auto checkpoint = p->checkpoint();
        if (checkpoint) {
            // Make sure that the checkpoint is also one of the parents of query
            distance_to_checkpoint = 1;
            const Query *q = query->parent().get();
            while (q != p && q != checkpoint) {
                ++distance_to_checkpoint;
                q = q->parent().get();
            }
            if (q == p) // (i.e. checkpoint not found)
                return false;
            break;
        }
    }

    // When query is easier than one of its parents, set the checkpoint of that parent to the query.
    // We want the oldest parent which is still younger or equal to the last checkpoint (to eliminate as many queries as possible)
    Query *parents[distance_to_checkpoint];
    Query *p = query.get();
    for (int i = 0; i < distance_to_checkpoint; ++i)
        parents[i] = (p = p->parent().get());
    for (int i = distance_to_checkpoint - 1; i >= 0; --i) {
        Query &parent = *parents[i];
        if (!parent.checkpoint() && is_easier_than(*query, parent) && parent.set_checkpoint(*query)) // TODO: multithreading can cause problems?
            break;
    }

    return true;
}

bool contains_in_order(const std::vector<FunctionRef> &v, const std::vector<FunctionRef> &w) {
    auto it_w = w.begin();
    for (auto it_v = v.begin(); it_v != v.end(); ++it_v) {
        while (true) {
            if (it_w == w.end())
                return false;
            if (*it_w == *it_v) {
                ++it_w;
                break;
            }
            ++it_w;
        }
    }
    return true;
}

bool Searcher::is_easier_than(const Query &p, const Query &q) {
    if (!m_searching)
        return false;

    const auto &p_variables = p.telescope().functions();
    const auto &q_variables = q.telescope().functions();

    if (p_variables.size() > q_variables.size()) // If p has more questions than q, return false
        return false;

    const auto &p_locals = p.locals();
    const auto &q_locals = q.locals();

    if (q_locals.size() > p_locals.size()) // If q has more local variables than p (i.e. more information), return false
        return false;

    // If p has a question that q does not have, return false
    if (!contains_in_order(p_variables, q_variables))
        return false;

    // If q has a local variable that p does not have, return false
    for (auto it_p = p_locals.begin(), it_q = q_locals.begin(); it_p < p_locals.end(); ++it_p, ++it_q) {
        if (!contains_in_order(*it_q, *it_p))
            return false;
    }

    return true;
}
