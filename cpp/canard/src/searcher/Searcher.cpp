//
// Created by Jesse Vogel on 01/07/2021.
//

#include "Searcher.h"
#include "../core/macros.h"
#include "../parser/Formatter.h"
#include <algorithm>

Searcher::Searcher(const std::unordered_set<Namespace *> &spaces,
                   const int max_depth,
                   const int max_threads) : m_max_depth(max_depth),
                                            m_index(spaces),
                                            m_thread_manager(max_threads),
                                            m_searching(false) {}

bool Searcher::search(const Telescope &telescope) {
    // Clear searcher
    clear();
    // Place initial query
    m_queue.push({std::make_shared<Query>(telescope), {}});
    // Reset counter
    m_counter = 1;
    // Create a pool of threads
    m_searching = true;
    m_thread_manager.start(&Searcher::search_loop, this);
    // Wait for all threads to end, and return
    m_thread_manager.join_all();
    return !m_result.empty();
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

        // Normalize the query before reducing: sort_and_convert parameters of telescope to local variables
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
            ++m_counter;
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
Searcher::search_helper(std::shared_ptr<Query> &query, const FunctionRef &thm, std::vector<std::shared_ptr<Query>> &reductions) {
    // Shortcut: if not searching anymore, just return false and be done with it
    if (!m_searching)
        return SEARCH_DONE;

    // If thm is excluded, return false
    if (thm == m_excluded_thm)
        return SEARCH_CONTINUE;

    // Try reducing query using thm
    auto sub_query = Query::reduce(query, thm);
    if (sub_query == nullptr)
        return SEARCH_CONTINUE;

    // If the sub_query is completely is_solved (i.e. no more telescope) we are done!
    if (sub_query->is_solved()) {
        m_mutex.lock();
        if (m_searching) {
            m_result = sub_query->final_solutions();
            m_searching = false;
        }
        m_mutex.unlock();
        return SEARCH_DONE;
    }

    // If the maximum depth was reached, omit this sub_query
    if (sub_query->depth() >= m_max_depth)
        return SEARCH_CONTINUE;

    // If this query strictly solves its parent, make it the only reduction
    if (injects_into(*sub_query, *query)) {
        reductions = {sub_query};
        return SEARCH_STOP;
    }

    // Add sub_query to queue
    reductions.push_back(std::move(sub_query));
    return SEARCH_CONTINUE;
}

void Searcher::clear() {
    m_queue = {};
    m_result.clear();
    m_counter = 0;
}

bool Searcher::check_reasonable(const std::shared_ptr<Query> &q, const std::shared_ptr<Query> &p) {
    // A query q is stupid with respect to a parent p if p (or one of its parents) injects into q.
    // This means that q is strictly more difficult to solve than p.
    if (p == nullptr) return true;
//    if (p->checkpoint()) return true; // TODO: only need to check up to the nearest checkpoint !
    if (injects_into(*p, *q)) return false;
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

    // When query injects into one of its parents, set the checkpoint of that parent to the query.
    // We want the oldest parent which is still younger or equal to the last checkpoint (to eliminate as many queries as possible)
    Query *parents[distance_to_checkpoint];
    Query *p = query.get();
    for (int i = 0; i < distance_to_checkpoint; ++i)
        parents[i] = (p = p->parent().get());
    for (int i = distance_to_checkpoint - 1; i >= 0; --i) {
        Query &parent = *parents[i];
        if (!parent.checkpoint() && injects_into(*query, parent) && parent.set_checkpoint(*query)) // TODO: multithreading can cause problems
            break;
    }

    return true;
}

bool Searcher::injects_into(const Query &p, const Query &q) const {
    // TODO: optimize this method ! Maybe its fine if its not 100% accurate
    // Goal is to map (injectively) all my indeterminates to other.telescope, but all other.locals to my locals
    // That is, we are checking if this query searches for less with more information.

    // Probably start at the end, work way towards the beginning
    // Method should be recursive
    // Can we just use Matchers ?

    // 1. let f = telescope[-1]
    // 2. for each g in other.telescope:
    //   2.1. Create sub_matcher with f -> g. If fail, continue with next g
    //   2.2. If succeeded, see what telescope are mapped, and update the lists of which to map and which are still allowed from other
    //   2.3. Continue recursively to the next (not yet mapped (also not induced mapped)) indeterminate (from the end, remember)

    // At any point, we need:
    // - current matcher (possibly null)
    // - telescope yet to be mapped
    // - allowed other.telescope to map to

//        System.out.println("Does [" + this + "] inject into [" + other + "] ? ");

    // Shortcut: if this is 'bigger' than other, it won't work
    if (p.telescope().size() > q.telescope().size())
        return false;

    return injects_into_helper(nullptr, p.telescope().functions(), q.telescope().functions());

    // TODO: actually, we would probably also need to check for the local variables..
}

bool Searcher::injects_into_helper(Matcher *matcher, std::vector<FunctionRef> unmapped, std::vector<FunctionRef> allowed) const {
    // Shortcut: if not searching anymore, just return false and be done with it
    if (!m_searching)
        return false;

    // Base case
    const size_t n = unmapped.size();
    if (n == 0)
        return true;

    // Starting at the back, try to map some unmapped Function to some allowed Function
    auto &f = unmapped.back();

    // If f is contained in allowed as well, we will assume that should be the assignment!
    // TODO: note: this is not efficient, filter out all duplicates in the beginning!
    auto it_f = std::find(allowed.begin(), allowed.end(), f);
    if (it_f != allowed.end()) {
        allowed.erase(it_f);
        unmapped.pop_back();
        return injects_into_helper(matcher, std::move(unmapped), std::move(allowed));
    }

    // Again starting at the back, find some allowed Function g to match our f
    const size_t m = allowed.size();
    for (int j = (int) m - 1; j >= 0; j--) {
        auto &g = allowed[j];
        bool valid_candidate = true;

        Matcher sub_matcher = (matcher == nullptr) ? Matcher(unmapped) : Matcher(matcher, unmapped);
        if (!sub_matcher.matches(f, g))
            continue;

        // Create new unmapped and allowed lists, by removing those which are mapped and to which they are mapped
        std::vector<FunctionRef> new_unmapped, new_allowed = allowed;
        for (auto &h: unmapped) {
            const auto &k = sub_matcher.get_solution(h);
            if (k == nullptr) {
                if (h == f)
                    return false; // TODO: it may happen that g -> f, so that matches returns true, but k == nullptr.

                // Add those which are not yet mapped
                new_unmapped.push_back(h);
            } else {
                // If h -> k, remove k from allowed lists
                // If list does not contain k, there was some invalid mapping anyway, so we continue
                auto it_k = std::find(new_allowed.begin(), new_allowed.end(), k);
                if (it_k == new_allowed.end()) {
                    valid_candidate = false;
                    break;
                }
                new_allowed.erase(it_k);
            }
        }
        // Now simply recursively find a match for the next unmapped
        if (valid_candidate && injects_into_helper(&sub_matcher, std::move(new_unmapped), std::move(new_allowed)))
            return true;
    }
    return false;
}
