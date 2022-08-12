//
// Created by Jesse Vogel on 01/07/2021.
//

#include "Searcher.h"
#include "../interpreter/Namespace.h"
#include "../core/macros.h"
#include <algorithm>

Searcher::Searcher(const int max_depth, const int max_threads) : m_max_depth(max_depth),
                                                                 m_thread_manager(max_threads) {}

void Searcher::add_namespace(Namespace &space) {
    // Make a list of lists of all functions that can be used during the search
    // We do this in advance so that we don't constantly create new arraylists
    auto theorems = space.context().functions();
    for (auto &entry: theorems)
        m_all_theorems.push_back(entry.second); // TODO: optimize ?
//        m_all_theorems.insert(m_all_theorems.end(), theorems.begin(), theorems.end());

    for (auto &entry: theorems) {
        const auto &thm = entry.second;
        const auto &thm_type_base = thm.type().base();

        // If the thmTypeBase is a dependency of the theorem, then store in the 'general' category
        auto &thm_parameters = thm->parameters().functions();
        if (std::find(thm_parameters.begin(), thm_parameters.end(), thm_type_base) != thm_parameters.end()) {
            m_generic_theorems.push_back(thm);
            continue;
        }

        auto it = m_index.find(thm_type_base);
        if (it != m_index.end()) {
            it->second.push_back(thm);
            continue;
        }

        // Alternatively, create a new entry in the index
        m_index.emplace(thm_type_base, std::vector<FunctionRef>({thm}));
    }
}

bool Searcher::search(const std::shared_ptr<Query> &query) {
    // Clear the vector of results
    m_result.clear();

    // Create a queue of queries for each depth (of queries).
    // The strategy is to first handle the low-depth queries, and then the higher-depth queries.
    m_depth_queues.clear();
    for (int depth = 0; depth < m_max_depth; ++depth)
        m_depth_queues.emplace_back();

    // The initial query contains depth 0
    m_depth_queues[0].push(query);

    // Create a pool of threads
    m_searching = true;
    m_thread_manager.start(&Searcher::search_loop, this);

    // Wait for all threads to end, and return
    m_thread_manager.join_all();
    return !m_result.empty();
}

void Searcher::search_loop() {
    while (m_searching) {
        // Take the first query from the first (non-empty) queue, i.e. the one with the lowest depth
        std::shared_ptr<Query> q = nullptr;
        m_mutex.lock();
        for (auto &queue: m_depth_queues) {
            if (!queue.empty()) {
                q = queue.front();
                queue.pop();
                break;
            }
        }
        m_mutex.unlock();

        // If there is no query, we should wait for other threads to come with updates
        // Depending on the result of the update, we should continue or stop
        if (q == nullptr) {
            if (m_thread_manager.wait_for_update()) continue; else break;
        }

        CANARD_DEBUG("Current query [" << q->to_string() << "]");

        // Normalize the query before reducing: convert parameters of indeterminates to local variables
        q = Query::normalize(q);

        // Check for redundancies
        if (is_redundant(q, q->parent())) continue;

        // Do some optimization TODO: this does not work correctly! TODO: watch out for multithreading!
//        optimize(q);

        // Now we are going to look for reductions. Before pushing to the queue, we will store all reductions
        // in a list `reductions`. Then we will sort this list based on the number of indeterminates, which is
        // some form of optimization
        std::vector<std::shared_ptr<Query>> reductions;

        // First list to search through is the list of local variables of q
        for (auto thm: q->locals()) {
            if (search_helper(q, thm, reductions))
                goto end;
        }

        // If the type base is an indeterminate of q, there is nothing better to do then to try all theorems
        FunctionRef h_type_base = q->last_indeterminate().type().base();
        if (q->is_indeterminate(h_type_base)) {
            for (auto &thm: m_all_theorems) {
                if (search_helper(q, thm, reductions))
                    goto end;
            }
        } else {
            // If the type base is known to the index, try the corresponding list of theorems
            auto it = m_index.find(h_type_base);
            if (it != m_index.end()) {
                for (auto &thm: it->second) {
                    if (search_helper(q, thm, reductions))
                        goto end;
                }
            }
            // Also try the general theorems
            for (auto &thm: m_generic_theorems) {
                if (search_helper(q, thm, reductions))
                    goto end;
            }
        }

        // Now sort the list of reductions based on the number of indeterminates
        std::sort(reductions.begin(), reductions.end(),
                  [](const std::shared_ptr<Query> &q1, const std::shared_ptr<Query> &q2) {
                      return q1->indeterminates_size() < q2->indeterminates_size();
                  });

        // Then add them to the queue
        m_mutex.lock();
        for (auto &r: reductions) {
            assert(r->depth() < m_max_depth);
            m_depth_queues[r->depth()].push(std::move(r));
        }
        m_mutex.unlock();

        // Notify the other threads for updates
        m_thread_manager.send_update(true);
    }
    end:

    // Send a permanent update
    m_thread_manager.send_permanent_update(true);

    // When we are out of the loop, this boolean makes the other threads terminate as well
    m_searching = false;

//    CANARD_LOG("END THREAD");
}

bool Searcher::search_helper(std::shared_ptr<Query> &query, FunctionRef &thm,
                             std::vector<std::shared_ptr<Query>> &reductions) {
    // Try reducing query using thm
    auto sub_query = Query::reduce(query, thm);
    if (sub_query == nullptr)
        return false;

    CANARD_DEBUG("Reduced query [" << sub_query->to_string() << "]");

    // If the sub_query is completely solved (i.e. no more indeterminates) we are done!
    if (sub_query->is_solved()) {
        m_mutex.lock();
        m_result = Query::final_solutions(sub_query);
        m_mutex.unlock();
        return true;
    }

    // If the maximum depth was reached, omit this sub_query
    if (sub_query->depth() >= m_max_depth)
        return false;

    // Add sub_query to queue
    reductions.push_back(std::move(sub_query));
    return false;
}

bool Searcher::is_redundant(const std::shared_ptr<Query> &q, const std::shared_ptr<Query> &p) {
    if (p == nullptr) return false;
    if (p->injects_into(q)) return true;
    return is_redundant(q, p->parent());
}

//void Searcher::optimize(const std::shared_ptr<Query> &q) {
//    // If sub_query injects into the parent p, then we can safely delete all other branches coming from that parent p
//    // since any solution of the parent will yield a solution for q
//    // TODO: reverse order of iterating, and whenever we have a match, break the for-loop ?
//    // TODO: this 'optimization' is not quite valid: if some parent gets 'strictly solved' by some query which was very slow (i.e. high depth), it should not be allowed to remove another query which already solved the query much more / earlier, just because it is a child of that parent..
//    for (auto p = q->parent(); p != nullptr; p = nullptr) { //p = p->parent()) {
//        if (!q->injects_into(p))
//            continue;
//
//        for (auto &queue: m_depth_queues) {
//            std::queue<std::shared_ptr<Query>> new_queue;
//            while (!queue.empty()) {
//                auto r = queue.front();
//                queue.pop();
//                bool should_remove = r->has_parent(p);
//
////                if (should_remove) // && r->injects_into(p)) // exception: if r is also a strict solution, don't remove r
////                    should_remove = false;
//
//                if (!should_remove)
//                    new_queue.push(std::move(r));
//                CANARD_DEBUG("Removed [" << r->to_string() << "] because its parent [" << p->to_string()
//                                         << "] was strictly solved by [" << q->to_string() << "]");
//            }
//            // Replace the old queue with the new queue (this works as queue is given by reference)
//            queue = std::move(new_queue);
//        }
//    }
//}
