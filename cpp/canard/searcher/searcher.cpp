//
// Created by Jesse Vogel on 01/07/2021.
//

#include "searcher.h"
#include <iostream>

Searcher::Searcher(const int max_depth) : m_max_depth(max_depth) {}

void Searcher::add_namespace(Namespace &space) {
    // Make a list of lists of all functions that can be used during the search
    // We do this in advance so that we don't constantly create new arraylists
    auto theorems = space.get_functions();
    m_all_theorems.insert(m_all_theorems.end(), theorems.begin(), theorems.end());

    for (FunctionPtr &thm : theorems) {
        auto thm_type_base = thm->get_type()->get_base();

        // If the thmTypeBase is a dependency of the theorem, then store in the 'general' category
        auto &thm_dependencies = thm->get_dependencies().m_functions;
        if (std::find(thm_dependencies.begin(), thm_dependencies.end(), thm_type_base) != thm_dependencies.end()) {
            m_generic_theorems.push_back(thm);
            continue;
        }

        auto it = m_index.find(thm_type_base);
        if (it != m_index.end()) {
            it->second.push_back(thm);
            continue;
        }

        // Alternatively, create a new entry in the index
        m_index.emplace(thm_type_base, std::vector<FunctionPtr>({thm}));
    }
}

bool Searcher::search(std::shared_ptr<Query> &query) {
    // Clear the queue of queries, and add the query we want to solve for
    m_queue = std::queue<std::shared_ptr<Query>>();
    m_queue.push(query);

    while (!m_queue.empty()) {
        // Take the first query from the queue, and try to reduce it
        // Note that we peek, and not poll, because we might need it for the next .next()
        auto q = m_queue.front();
        m_queue.pop();

//        std::cerr << "current query [" << q->to_string() << "]" << std::endl;

        // Normalize the query before reducing: convert dependencies of indeterminates to local variables
        q = Query::normalize(q);

        // Check for redundancies
        if (is_redundant(q, q->get_parent()))
            continue;

        // If subQuery injects into the parent p, then we can safely delete all other branches coming from that parent p
        // since any solution of the parent will yield a solution for q
        // TODO: reverse order of iterating, and whenever we have a match, break the for-loop
        for (std::shared_ptr<Query> p = q->get_parent(); p != nullptr; p = p->get_parent()) {
            if (q->injects_into(p)) {
                std::queue<std::shared_ptr<Query>> new_queue;
                while (!m_queue.empty()) {
                    auto r = m_queue.front();
                    m_queue.pop();
                    bool should_remove = false;
                    for (auto s = r->get_parent(); s != nullptr; s = s->get_parent()) {
                        if (s == p) {
                            should_remove = true;
                            break;
                        }
                    }
                    if (!should_remove)
                        new_queue.push(std::move(r));
                }
                m_queue = std::move(new_queue);
            }
        }

        // First list to search through is the list of local variables of q
        for (auto thm : q->get_locals())
            if (search_helper(q, thm)) return true;

        // If the type base is an indeterminate of q, there is nothing better to do then to try all theorems
        FunctionPtr h_type_base = q->get_last_indeterminate()->get_type()->get_base();
        if (q->is_indeterminate(h_type_base)) {
            for (auto &thm : m_all_theorems)
                if (search_helper(q, thm)) return true;
            continue;
        }

        // If the type base is known to the index, try the corresponding list of theorems
        auto it = m_index.find(h_type_base);
        if (it != m_index.end()) {
            for (auto &thm : it->second)
                if (search_helper(q, thm)) return true;
        }

        // Also try the general theorems
        for (auto &thm : m_generic_theorems)
            if (search_helper(q, thm)) return true;
    }

    return false;
}

bool Searcher::search_helper(std::shared_ptr<Query> &query, FunctionPtr &thm) {
    // Try reducing query using thm
    auto sub_query = Query::reduce(query, thm);
    if (sub_query == nullptr)
        return false;

//    std::cerr << "reduced query [" << sub_query->to_string() << "]" << std::endl;

    // If the sub_query is completely solved (i.e. no more indeterminates) we are done!
    if (sub_query->is_solved()) {
        m_result = Query::get_final_solutions(sub_query);
        return true;
    }

    // If the maximum depth was reached, omit this sub_query
    if (sub_query->get_depth() >= m_max_depth)
        return false;

    // Add sub_query to queue
    m_queue.push(sub_query);
    return false;
}

bool Searcher::is_redundant(const std::shared_ptr<Query> &q, const std::shared_ptr<Query> &p) {
    if (p == nullptr) return false;
    if (p->injects_into(q)) return true;
    return is_redundant(q, p->get_parent());
}
