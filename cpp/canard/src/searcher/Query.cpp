//
// Created by Jesse Vogel on 01/07/2021.
//

#include "Query.h"
#include "../core/macros.h"
#include "../parser/Formatter.h"
#include <utility>
#include <sstream>
#include <algorithm>
#include <numeric>

int max(const std::vector<int> &vector) {
    int max = 0;
    for (const int &x: vector) {
        if (x > max)
            max = x;
    }
    return max;
}

Query::Query(Telescope telescope) :
        m_parent(nullptr),
        m_telescope(std::move(telescope)),
        m_depths(m_telescope.size(), 0),
        m_locals_depths(m_telescope.size(), 0),
        m_depth(compute_depth()),
        m_complexity(compute_complexity()) {}

Query::Query(std::shared_ptr<Query> parent,
             Telescope telescope,
             std::vector<int> depths,
             std::vector<std::vector<FunctionRef>> locals,
             std::vector<int> locals_depths,
             std::unordered_map<FunctionRef, FunctionRef> solutions)
        : m_parent(std::move(parent)),
          m_telescope(std::move(telescope)),
          m_depths(std::move(depths)),
          m_locals(std::move(locals)),
          m_locals_depths(std::move(locals_depths)),
          m_solutions(std::move(solutions)),
          m_depth(compute_depth()),
          m_complexity(compute_complexity()) {}

std::shared_ptr<Query> Query::normalize(const std::shared_ptr<Query> &query) {
    // Get goal
    int h_index;
    const auto &h = query->goal(&h_index);
    // If there is no goal, or there are no parameters, nothing to do here
    if (h == nullptr || h->parameters().empty())
        return query;

    // Create a sub_query where h is replaced by a parameter-less version of h
    // This introduces new local functions: the parameters of h
    std::vector<FunctionRef> new_functions = query->telescope().functions();
    auto new_h = Function::make({}, h.type());
    new_functions[h_index] = new_h;

    // Add a layer of local functions given by the parameters of h
    std::vector<std::vector<FunctionRef>> new_locals = query->m_locals;
    new_locals.push_back(h->parameters().functions());

    std::vector<int> new_locals_depths = query->m_locals_depths;
    new_locals_depths[h_index]++; // increase locals depth of new_h by one

    std::shared_ptr<Query> sub_query(new Query(
            query,
            Telescope(new_functions),
            query->m_depths, // use the same depths, seems fair
            std::move(new_locals),
            std::move(new_locals_depths),
            {{h, new_h}} // note: parameters don't agree, but that is fixed in final_solutions method
    ));

    return sub_query;
}

std::shared_ptr<Query> Query::reduce(const std::shared_ptr<Query> &query, const FunctionRef &thm) {
    // Get goal
    int h_index;
    const auto &h = query->goal(&h_index);
    // At this point, h is asserted to have no parameters!
    CANARD_ASSERT(h->parameters().empty(), "h cannot have parameters");
    CANARD_ASSERT(query->m_locals_depths[h_index] == query->m_locals.size(), "h should have maximal context depth");

    // Create matcher with indeterminates from both telescope and thm parameters
    const auto &telescope = query->telescope().functions();
    const auto &thm_parameters = thm->parameters().functions();
    std::vector<FunctionRef> indeterminates_total = telescope;
    indeterminates_total.insert(indeterminates_total.end(), thm_parameters.begin(), thm_parameters.end());
    Matcher matcher(indeterminates_total);

    // Note: we do not match h with thm, since they obviously need not match.
    // Instead, we match h.type() with thm.type() to see if thm can be applied to get a solution for h
    if (!matcher.matches(h.type(), thm.type()))
        return nullptr;

    // Create the new sub_query, and keep track of the solutions (for telescope) and the arguments (for thm)
    // and the new telescope (for the sub_query)
    std::vector<FunctionRef> new_indeterminates;
    auto max_indeterminates = telescope.size() - 1 + thm_parameters.size(); // maximum number of new indeterminates: prioritize speed over memory
    new_indeterminates.reserve(max_indeterminates);
    std::unordered_map<FunctionRef, FunctionRef> new_solutions;
    std::vector<int> new_depths, new_locals_depths;
    new_depths.reserve(max_indeterminates);
    new_locals_depths.reserve(max_indeterminates);
    std::vector<FunctionRef> thm_arguments(thm_parameters.size());
    std::vector<std::vector<FunctionRef>> new_locals(query->m_locals.size());
    int locals_size = 0;
    for (int i = 0; i < query->m_locals.size(); ++i) {
        int size = (int) query->m_locals[i].size();
        new_locals[i].reserve(size);
        locals_size += size;
    }

    const int h_depth = query->m_depths[h_index];
    const int h_context_depth = query->m_locals_depths[h_index];

    // Local functions, telescope functions and thm parameters can all depend on one another, so they must be carefully mapped to the new query.
    // Let `mappable_old` be all of these functions (except for h, it will be treated later on).
    // Notes:
    //  - sometimes functions must be cloned (because their signature depends on functions that have solutions)
    //  - those functions that have solutions, we call 'infected', so that we only clone those whose signature depends on an 'infected' function
    //  - those functions become infected then as well
    //  - we first insert local functions (as rather have that something has a local function as solution than the other way around)
    //  - we keep track of context depth, it should be non-decreasing along the way
    //  - the order (1) local functions, (2) telescope functions, (3) thm parameters is important!
    std::vector<FunctionRef> mappable;
    mappable.reserve(telescope.size() - 1 + thm_parameters.size() + locals_size);
    std::vector<std::vector<FunctionRef>> unmapped_locals = query->m_locals;
    std::vector<int> unmapped_telescope_indices;
    unmapped_telescope_indices.reserve(telescope.size() - 1);
    std::vector<int> unmapped_thm_parameter_indices;
    unmapped_thm_parameter_indices.reserve(thm_parameters.size());
    for (const auto &locals: query->m_locals)
        mappable.insert(mappable.end(), locals.begin(), locals.end()); // locals might need mapping too
    for (int i = 0; i < telescope.size(); ++i) {
        const auto &f = telescope[i];
        if (f->is_base() && f != h) {
            mappable.push_back(f);
            unmapped_telescope_indices.push_back(i);
        }
    }
    for (int i = 0; i < thm_parameters.size(); ++i) {
        const auto &f = thm_parameters[i];
        if (f->is_base()) {
            mappable.push_back(f);
            unmapped_thm_parameter_indices.push_back(i);
        }
    }
    // Create matcher
    Matcher query_to_sub_query(mappable);

    // Create sets of unmapped functions and infected functions. We choose sets because we want to check membership fast.
    std::vector<FunctionRef> unmapped = mappable;
    std::vector<FunctionRef> infected; // indicates which functions have a (non-trivial) solution
    infected.reserve(mappable.size());

    int locals_depth_tracker = 0; // the way in which the mappable are resolved must be w.r.t. non-decreasing locals depth, as otherwise there are non-allowed solutions

    while (!unmapped.empty()) {
        // (1) Local functions
        for (int depth = 0; depth < unmapped_locals.size(); ++depth) {
            auto &vector = unmapped_locals[depth];
            for (auto it = vector.begin(); it != vector.end(); ++it) {
                const auto &f = *it;
                if (f.signature_depends_on(unmapped))
                    continue;

                // Clone the local function if it depends on an infected function
                auto g = (!infected.empty() && f.signature_depends_on(infected))
                         ? query_to_sub_query.clone(f)
                         : f;

                // ... and if so, match, set solution, and mark as infected
                if (g != f) {
                    query_to_sub_query.assert_matches(f, g);
                    new_solutions.emplace(f, g);
                    infected.push_back(f);
                }

                new_locals[depth].push_back(g);
                unmapped.erase(std::find(unmapped.begin(), unmapped.end(), f));
                vector.erase(it);
                goto continue_while;
            }
        }

        // (2) Telescope functions
        for (auto it = unmapped_telescope_indices.begin(); it != unmapped_telescope_indices.end(); ++it) {
            const int i = *it;
            const auto &f = telescope[i];
            auto f_solution = matcher.get_solution(f);
            if (f_solution != nullptr) {
                // If f has solution, the solution must not depend on any unmapped indeterminate
                if (f_solution.depends_on(unmapped))
                    continue;

                // Check if `f_solution` is defined in the local context of f
                const int f_context_depth = query->m_locals_depths[i];
                if (!query->is_allowed_solution(f_context_depth, f_solution))
                    return nullptr;

                // Convert solution along matcher to get actual solution
                f_solution = query_to_sub_query.convert(f_solution);
            } else {
                // If f has no solution, f will remain an indeterminate, so we duplicate it to the new query
                // But first, the signature of f may not depend on unmapped functions, otherwise we don't know what to create yet!
                if (f.signature_depends_on(unmapped))
                    continue;

                // This can be done cheaply, i.e. might preserve f as an indeterminate
                f_solution = (!infected.empty() && f.signature_depends_on(infected))
                             ? query_to_sub_query.clone(f)
                             : f;

                new_indeterminates.push_back(f_solution);
                new_depths.push_back(query->m_depths[i]); // same depth as f
                new_locals_depths.push_back(query->m_locals_depths[i]); // same context depth as f
            }

            // Local depth condition
            if (query->m_locals_depths[i] < locals_depth_tracker)
                return nullptr;
            locals_depth_tracker = query->m_locals_depths[i];

            // Store solution, remove from unmapped, assert match, and signal changes
            if (f_solution != f) {
                new_solutions.emplace(f, f_solution);
                infected.push_back(f);
            }
            query_to_sub_query.assert_matches(f, f_solution);
            unmapped.erase(std::find(unmapped.begin(), unmapped.end(), f));
            unmapped_telescope_indices.erase(it);
            goto continue_while;
        }

        // (3) Thm parameters
        for (auto it = unmapped_thm_parameter_indices.begin(); it != unmapped_thm_parameter_indices.end(); ++it) {
            const int i = *it;
            const auto &f = thm_parameters[i];
            auto argument = matcher.get_solution(f);
            if (argument != nullptr) {
                // If the argument for f is determined, this argument may not depend on unmapped functions
                if (argument.depends_on(unmapped))
                    continue;

                // Convert argument along matcher to get actual argument
                argument = query_to_sub_query.convert(argument);
            } else {
                // If there is no argument for f, then f becomes a new indeterminate (after being cloned)
                if (f.signature_depends_on(unmapped))
                    continue;

                // To find the context depth of a thm parameter is a bit tricky.
                // If we reached this point in the code, then all the functions in unmapped_telescope depend on some thm parameter.
                // If those functions depend on some thm parameter, then the local depth of that thm parameter can be at most the local depth of that function.
                // TODO: can a similar thing happen with the local variables ?
                int f_context_depth = h_context_depth; // by default, give it the maximum possible context depth
                for (const int &j: unmapped_telescope_indices) {
                    const auto &g = telescope[j];
                    const auto &g_solution = matcher.get_solution(g);
                    if (g_solution ? g_solution.depends_on({f}) : g.signature_depends_on({f})) {
                        f_context_depth = query->m_locals_depths[j];
                        break; // since context depths are non-decreasing, we can simply break here
                    }
                }

                // Context depth check
                if (f_context_depth < locals_depth_tracker)
                    return nullptr;
                locals_depth_tracker = f_context_depth;

                // Note: no cheap clone for thm parameters, as theorems may get applied multiple times, so we need an actual clone
                argument = query_to_sub_query.clone(f);

                new_indeterminates.push_back(argument);
                new_depths.push_back(h_depth + 1); // indeterminate was introduced because of h, so h_depth + 1
                new_locals_depths.push_back(f_context_depth);
            }

            // Store solution, remove from unmapped, assert match, and signal changes
            infected.push_back(f);
            thm_arguments[i] = argument;
            query_to_sub_query.assert_matches(f, argument);
            unmapped.erase(std::find(unmapped.begin(), unmapped.end(), f));
            unmapped_thm_parameter_indices.erase(it);
            goto continue_while;
        }

        // (4) Break
        break;

        continue_while:;
    }

    // If there are still unmapped functions, there must be some circular dependence, and we concede
    if (!unmapped.empty()) {
        CANARD_DEBUG("circular dependence detected");
        return nullptr;
    }

    // Convert the thm arguments that were specializations
    for (auto &f: thm_arguments) {
        if (!f->is_base())
            f = query_to_sub_query.convert(f);
    }

    // Set solution for h as specialization of thm
    try {
        // Note that thm itself must also be converted, since it might have changed along the way to sub_query!
        // E.g. for something like (not precisely) `search (P {A B : Prop} (h (a : A) : B) (x : A) : B)`
        auto h_solution = query_to_sub_query.convert(thm).specialize({}, std::move(thm_arguments));
        new_solutions.emplace(h, std::move(h_solution));
    } catch (SpecializationException &e) {
        CANARD_ASSERT(false, "Ai ai ai! Not what was supposed to happen!");
    }

    // Create the reduced query based on the solutions and new indeterminates
    const int new_context_depth = max(new_locals_depths);
    new_locals.erase(new_locals.begin() + new_context_depth, new_locals.end());
    std::shared_ptr<Query> sub_query(new Query(
            query,
            Telescope(new_indeterminates),
            std::move(new_depths),
            std::move(new_locals),
            std::move(new_locals_depths),
            std::move(new_solutions)
    ));

    return sub_query;
}

std::vector<FunctionRef> Query::final_solutions() const {
    // This can only be done if this query is solved
    CANARD_ASSERT(is_solved(), "cannot call final_solutions on unsolved query");

    // Create chain of queries, starting at the bottom, all the way up to the top
    // Note: we include every query which contains a parent, so the initial query will not be included
    std::vector<const Query *> chain;
    for (const Query *query = this; query->parent() != nullptr; query = query->parent().get())
        chain.push_back(query);
    const Query &initial_query = *(chain.back()->parent());

    /* The idea is as follows: starting at the back, `matcher` will functions from Query ..., 4, 3, 2, 1 to the solution,
       at each step using the previous matcher

        Query 1 -> Query 2 -> Query 3 -> Query 4
            \           \          \       |
             ----->      ----->     -> Solution
    */

    const auto n = chain.size();
    std::vector<std::unique_ptr<Matcher>> matchers;
    std::vector<std::unique_ptr<std::vector<FunctionRef>>> indeterminates;
    for (int i = 0; i < n; ++i) {
        auto &query = chain[i];
        CANARD_ASSERT(query->parent() != nullptr, "Chain should only contain queries which have a parent"); // (just to be sure)

        // The indeterminates of the matcher are the keys of the solution set of the query
        auto next_indeterminates = std::unique_ptr<std::vector<FunctionRef>>(new std::vector<FunctionRef>());
        next_indeterminates->reserve(query->m_solutions.size());
        for (auto &entry: query->m_solutions)
            next_indeterminates->push_back(entry.first);
        auto next_matcher = std::unique_ptr<Matcher>((i == 0)
                                                     ? new Matcher(*next_indeterminates)
                                                     : new Matcher(matchers.back().get(), *next_indeterminates));
        indeterminates.push_back(std::move(next_indeterminates));

        const auto &h = query->parent()->goal();
        for (auto &entry: query->m_solutions) {
            const auto &f = entry.first;
            auto f_solution = entry.second;

            // First convert/clone the solution along the previous matcher
            if (i > 0) {
                f_solution = matchers.back()->convert(f_solution);
                // If the converted `f_solution` has a signature depending on some indeterminate of a previous matcher, then this means `f_solution`
                // is (some cloned version of a) parameter, and hence it must be cloned instead of converted. (HOPEFULLY ALL GOES WELL...)
                for (const auto &m: matchers) {
                    if (f_solution.signature_depends_on(m->indeterminates())) {
                        f_solution = matchers.back()->clone(f_solution);
                        break;
                    }
                }
            }

            // Case f = h and h has parameters, we must reintroduce the parameters again
            if (f == h && !h->parameters().empty()) {
                try {
                    // Note: the parameters of h have become local variables in the subquery. Therefore, they could have been solved for
                    // which means we need to convert them along the matcher before we can use them as parameters again.
                    auto h_parameters_converted = Telescope(matchers.back()->convert(h->parameters().functions()));
                    f_solution = f_solution.specialize(h_parameters_converted, {});
                }
                catch (SpecializationException &e) {
                    CANARD_ASSERT(false, e.m_message);
                }
            }

            // Now the next matcher should match f to f_solution
            next_matcher->assert_matches(f, f_solution);
        }
        // Add to chain of matchers
        matchers.push_back(std::move(next_matcher));
    }
    // Finally, we can sort_and_convert the telescope of the initial query
    return matchers.back()->convert(initial_query.telescope().functions());
}

bool Query::is_allowed_solution(const int context_depth, const FunctionRef &solution) {
    CANARD_ASSERT(context_depth <= m_locals.size(), "context_depth > m_locals.size()");
    // A solution is allowed only if it does not depend on contexts deeper than context_depth
    for (auto it = m_locals.begin() + context_depth; it != m_locals.end(); ++it) {
        if (solution.depends_on(*it))
            return false;
    }
    return true;
}

const FunctionRef &Query::goal(int *index) const {
    // Return the last base function of the telescope, and give index
    for (auto it = m_telescope.functions().rbegin(); it != m_telescope.functions().rend(); ++it) {
        const auto &f = *it;
        if (f->is_base()) {
            if (index)
                *index = (int) std::distance(m_telescope.functions().begin(), it.base()) - 1;
            return f;
        }
    }
    return FunctionRef::null();
}

int Query::compute_complexity() const {
    // This is very hard to get optimal of course ...
    int cost = 0;
    for (int i = 0; i < m_telescope.size(); ++i) {
        cost += 1;
        cost += m_depths[i];
        cost += m_locals_depths[i];
    }
    cost += (int) (10000 * telescope().size());
//    cost += (int) (100 * max(m_depths));
//    cost += (int) (10 * m_locals.size()); // prevents e.g. unnecessarily nested `modus_tollens`
    return cost;
}

int Query::compute_depth() const {
    return max(m_depths);
}

bool Query::set_checkpoint(const Query &other) {
    if (m_checkpoint == nullptr) {
        m_checkpoint = &other;
        return true;
    }
    return false;
}

int Query::distance_to_checkpoint() const {
    int distance = 0;
    for (const Query *p = parent().get(); p != nullptr; p = p->parent().get()) {
        ++distance;
        if (p->checkpoint()) {
            // Make sure that the checkpoint is also one of the parents of query
            distance = 1;
            const Query *q = parent().get();
            while (q != p && q != p->checkpoint()) {
                ++distance;
                q = q->parent().get();
            }
            if (q == p) // (i.e. checkpoint not found)
                return distance;
        }
    }
    return distance;
}
