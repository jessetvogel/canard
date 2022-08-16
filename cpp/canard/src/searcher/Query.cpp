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
        m_depth(max(m_depths)),
        m_context_depths(m_telescope.size(), 0) {
    m_own_context = std::unique_ptr<Context>(new Context());
    m_context = m_own_context.get();
    m_context_depth = 0;
}

Query::Query(std::shared_ptr<Query> parent, Telescope telescope, std::vector<int> depths, std::vector<int> context_depths,
             std::unordered_map<FunctionRef, FunctionRef> solutions)
        : m_parent(std::move(parent)),
          m_telescope(std::move(telescope)),
          m_depths(std::move(depths)),
          m_depth(max(m_depths)),
          m_context_depths(std::move(context_depths)),
          m_solutions(std::move(solutions)) {
    m_own_context = nullptr;
    m_context = m_parent->m_context;
    m_context_depth = m_parent->m_context_depth;
}

void Query::create_own_context() {
    CANARD_ASSERT(m_own_context == nullptr, "query already has own context");
    Context &parent = *m_context;
    m_own_context = std::unique_ptr<Context>(new Context(parent));
    m_context = m_own_context.get();
    ++m_context_depth;
}

void Query::escape_to_context_depth(int context_depth) {
    CANARD_ASSERT(m_context_depth >= context_depth, "cannot escape from context depth " << m_context_depth << " to " << context_depth);
    while (m_context_depth > context_depth) {
        m_context = m_context->parent();
        --m_context_depth;
    }
    m_own_context = nullptr;
}

std::shared_ptr<Query> Query::normalize(const std::shared_ptr<Query> &query) {
    // Get last function of the telescope
    const auto &h = query->goal();
    CANARD_ASSERT(h->is_base(), "h must be a base function");
    // If h contains no parameters, nothing to do here
    if (h->parameters().empty())
        return query;

    // Create a sub_query where h is replaced by a parameter-less version of h
    // This introduces new local functions: the parameters of h
    std::vector<FunctionRef> new_functions = query->telescope().functions();
    auto new_h = Function::make({}, h.type());
    new_functions.back() = new_h;

    std::vector<int> new_context_depths = query->m_context_depths;
    new_context_depths.back()++; // increase context depth of new_h by one

    std::shared_ptr<Query> sub_query(new Query(
            query,
            Telescope(new_functions),
            query->m_depths, // use the same depths, seems fair
            std::move(new_context_depths),
            {{h, new_h}} // TODO: look at this some time again, as parameters don't match..
    ));

    // Create a sub-context filled with the parameters of h; these can then be used to solve for new_h
    sub_query->create_own_context();
    for (const auto &f: h->parameters().functions())
        sub_query->context().put(f);

    return sub_query;
}

std::shared_ptr<Query> Query::reduce(const std::shared_ptr<Query> &query, const FunctionRef &thm) {
    // Get last function of the telescope
    const auto &h = query->goal();
    // At this point, h is asserted to have no parameters!
    CANARD_ASSERT(h->parameters().empty(), "h cannot have parameters");
    CANARD_ASSERT(h->is_base(), "h must be a base function");
    if (query->m_context_depths.back() != query->m_context_depth) {
        Formatter formatter;
        CANARD_LOG(formatter.to_string_tree(*query));
        CANARD_ASSERT(false, "h should have maximal context depth");
    }

    // Create matcher with indeterminates from both telescope and thm parameters
    std::vector<FunctionRef> all_indeterminates = query->telescope().functions();
    const auto &thm_parameters = thm->parameters().functions();
    all_indeterminates.insert(all_indeterminates.end(), thm_parameters.begin(), thm_parameters.end());
    Matcher matcher(all_indeterminates);

    // Note: we do not match h with thm, since they obviously need not match.
    // Instead, we match h.type() with thm.type() to see if thm can be applied to get a solution for h
    if (!matcher.matches(h.type(), thm.type()))
        return nullptr;

#ifdef DEBUG
    Formatter formatter;
    CANARD_DEBUG("Solved for " << formatter.to_string_full(h) << " with " << formatter.to_string_full(thm) << " in " << formatter.to_string(*query));
#endif

    // Create the new sub_query, and keep track of the solutions (for telescope) and the arguments (for thm)
    // and the new telescope (for the sub_query)
    std::vector<FunctionRef> new_indeterminates;
    std::unordered_map<FunctionRef, FunctionRef> new_solutions;
    std::vector<int> new_depths, new_context_depths;
    std::vector<FunctionRef> thm_arguments(thm_parameters.size());

    const int h_depth = query->m_depths.back();
    const int h_context_depth = query->m_context_depths.back();

    std::vector<FunctionRef> mappable = all_indeterminates;

    // The motivation here is that NOTHING should depend on h, as it is the last indeterminate.
    // It might happen that some local variable is depends on h, but in that case we should just throw it away
    // (because it cannot be that another indeterminate will depend on that local variable, as it would then in turn
    // depend on h again). So in conclusion, we can just forget about h for now, and then in the end construct
    // the value/solution for h!
    mappable.erase(std::find(mappable.begin(), mappable.end(), h));
    // mappable.insert(mappable.end(), query->m_locals.begin(), query->m_locals.end()); // locals might need mapping too! TODO: why? ?

    Matcher query_to_sub_query(mappable);

    std::vector<FunctionRef> unmapped = mappable;
    std::vector<int> unmapped_telescope_indices(query->telescope().size() - 1); // h is treated later/differently (TODO: right?)
    std::vector<int> unmapped_thm_parameter_indices(thm->parameters().size());
    std::iota(unmapped_telescope_indices.begin(), unmapped_telescope_indices.end(), 0);
    std::iota(unmapped_thm_parameter_indices.begin(), unmapped_thm_parameter_indices.end(), 0);

    int context_depth_tracker = 0; // the way in which the unmapped are resolved must be w.r.t. non-decreasing context depth, as otherwise there are non-allowed solutions

    bool changes = true;
    while (changes && !(unmapped_telescope_indices.empty() && unmapped_thm_parameter_indices.empty())) {
        changes = false;
        // Telescope functions
        for (auto it = unmapped_telescope_indices.begin(); it != unmapped_telescope_indices.end(); ++it) {
            const int i = *it;
            const auto &f = query->telescope().functions()[i];
            auto f_solution = matcher.get_solution(f); // TODO: this assumes f is a base function, what if it isn't ?
            if (f_solution != nullptr) {
                // If f has solution, the solution must not depend on any unmapped indeterminate
                if (f_solution->depends_on(unmapped))
                    continue;

                // Check if `f_solution` is defined in the local context of f
                const int f_context_depth = query->m_context_depths[i];
                if (!query->is_allowed_solution(f_context_depth, f_solution)) {
#ifdef DEBUG
                    CANARD_DEBUG(formatter.to_string_full(f_solution) << " is not an allowed  solution for " << formatter.to_string_full(f));
#endif
                    return nullptr;
                }

                // Convert solution along matcher to get actual solution
                f_solution = query_to_sub_query.convert(f_solution);
            } else {
                // If f has no solution, f will remain an indeterminate, so we duplicate it to the new query
                // But first, the signature of f may not depend on unmapped functions, otherwise we don't know what to create yet!
                if (f->signature_depends_on(unmapped))
                    continue;

                // This can be done cheaply, i.e. might preserve f as an indeterminate
                f_solution = query_to_sub_query.clone_cheaply(f); // TODO: is it faster to just clone always ? profile this
                // TODO: also, sometimes it can be done even cheaper, f may depend on indeterminates of query_to_sub_query, but they need not have solutions!

                new_indeterminates.push_back(f_solution);
                new_depths.push_back(query->m_depths[i]); // same depth as f
                new_context_depths.push_back(query->m_context_depths[i]); // same context depth as f
            }

            if (query->m_context_depths[i] < context_depth_tracker)
                return nullptr;
            context_depth_tracker = query->m_context_depths[i];

            // Store solution, remove from unmapped, assert match, and signal changes
            if (f_solution != f)
                new_solutions.emplace(f, f_solution);
            query_to_sub_query.assert_matches(f, f_solution);
            unmapped.erase(std::find(unmapped.begin(), unmapped.end(), f)); // TODO: seems slow?
            it = unmapped_telescope_indices.erase(it);
            --it; // TODO: technically undefined behaviour
            changes = true;
            goto end_while;
        }
        // Thm parameters
        for (auto it = unmapped_thm_parameter_indices.begin(); it != unmapped_thm_parameter_indices.end(); ++it) {
            const int i = *it;
            const auto &f = thm_parameters[i]; // TODO: what if the thm parameter f is a specialization ?
            auto argument = matcher.get_solution(f);
            if (argument != nullptr) {
                // If the argument for f is determined, this argument may not depend on unmapped functions
                if (argument->depends_on(unmapped))
                    continue;

                // TODO: this should never happen right ? h should be allowed to anything, as it is the last indeterminate
//                    // Check whether `argument` is allowed (w.r.t. the local variables)

                // Convert argument along matcher to get actual argument
                argument = query_to_sub_query.convert(argument);
            } else {
                // If there is no argument for f, then f becomes a new indeterminate (after being cloned)
                if (f->signature_depends_on(unmapped))
                    continue;

                // Note: no cheap clone for thm parameters, as theorems may get applied multiple times, so we need an actual clone
                argument = query_to_sub_query.clone(f);

                new_indeterminates.push_back(argument);
                new_depths.push_back(h_depth + 1); // indeterminate was introduced because of h, so h_depth + 1
                new_context_depths.push_back(h_context_depth); // similarly, h_context_depth

                // TODO: again, these should be all, as h is allowed to anything!
//                    // This new indeterminate may depend on the local variables that h was allowed to depend on
            }

            if (h_context_depth < context_depth_tracker)
                return nullptr;
            context_depth_tracker = h_context_depth;

            // Store solution, remove from unmapped, assert match, and signal changes
            CANARD_ASSERT(argument != f, "argument cannot be f"); // TODO: why would it ever be ?
            thm_arguments[i] = argument;
            query_to_sub_query.assert_matches(f, argument);
            unmapped.erase(std::find(unmapped.begin(), unmapped.end(), f)); // TODO: seems slow?
            it = unmapped_thm_parameter_indices.erase(it);
            --it; // TODO: technically undefined behaviour
            changes = true;
            goto end_while;
        }
        end_while:;
    }

    // If there are still unmapped functions, there must be some circular dependence, and we concede
    if (!(unmapped_telescope_indices.empty() && unmapped_thm_parameter_indices.empty())) {
        CANARD_DEBUG("circular dependence detected");
        return nullptr;
    }

    // Set solution for h as specialization of thm
    try {
        // Note that thm itself must also be converted, since it might have changed along the way to sub_query!
        // E.g. for `search (P {A B : Prop} (h (a : A) : B) (x : A) : B)` // TODO: really ? I don't see how ?
        auto h_solution = query_to_sub_query.convert(thm).specialize({}, std::move(thm_arguments));
        new_solutions.emplace(h, std::move(h_solution));
    } catch (SpecializationException &e) {
        CANARD_ASSERT(false, "Ai ai ai! Not what was supposed to happen!");
    }

    // Create the reduced query based on the solutions and new indeterminates
    const int new_context_depth = max(new_context_depths);
    std::shared_ptr<Query> sub_query(new Query(
            query,
            Telescope(new_indeterminates),
            std::move(new_depths),
            std::move(new_context_depths),
            std::move(new_solutions)
    ));
    sub_query->escape_to_context_depth(new_context_depth);

    return sub_query;
}

std::vector<FunctionRef> Query::final_solutions() {
    // This can only be done if this query is solved
    CANARD_ASSERT(is_solved(), "cannot call final_solutions on unsolved query");

    // Create chain of queries, starting at the bottom, all the way up to the top
    // Note: we include every query which contains a parent, so the initial query will not be included
    std::vector<Query *> chain;
    for (Query *query = this; query->parent() != nullptr; query = query->parent().get())
        chain.push_back(query);
    const Query &initial_query = *(chain.back()->parent());

    /* The idea is as follows: starting at the back, `matcher` will map from Query ..., 4, 3, 2, 1 to the solution,
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
        for (auto &entry: query->m_solutions)
            next_indeterminates->push_back(entry.first);
        auto next_matcher = std::unique_ptr<Matcher>((i == 0)
                                                     ? new Matcher(*next_indeterminates)
                                                     : new Matcher(matchers.back().get(), *next_indeterminates));
        indeterminates.push_back(std::move(next_indeterminates));

        const auto &h = query->parent()->telescope().functions().back();
        for (auto &entry: query->m_solutions) { // TODO: reason is that locals can also be mapped to solutions, and potentially not all telescope need to have solutions at each step ?
            const auto &f = entry.first;
            auto f_solution = entry.second;

            // First convert the solution along the previous matcher
            if (i > 0)
                f_solution = matchers.back()->convert(f_solution);

            // Case f = h and h has parameters, we must reintroduce the parameters again
            // TODO: should these parameters be cloned again, or are they safe to use ?
            if (f == h && !h->parameters().empty()) {
                try {
                    f_solution = f_solution.specialize(h->parameters(), {});
                }
                catch (SpecializationException &e) {
                    CANARD_ASSERT(false, e.m_message);
                }
            }
            // Now the next matcher should map f to f_solution
            next_matcher->assert_matches(f, f_solution);
        }
        // Add to chain of matchers
        matchers.push_back(std::move(next_matcher));
    }
    // Finally, we can convert the telescope of the initial query
    return matchers.back()->convert(initial_query.telescope().functions());
}

bool Query::injects_into(const std::shared_ptr<Query> &other) {
    assert(other != nullptr);
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
    if (telescope().size() > other->telescope().size())
        return false;

    return injects_into_helper(nullptr, telescope().functions(), other->telescope().functions());

    // TODO: actually, we would probably also need to check for the local variables..
}

bool Query::injects_into_helper(Matcher *matcher, std::vector<FunctionRef> unmapped, std::vector<FunctionRef> allowed) {
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
    const auto m = (int) allowed.size();
    for (int j = m - 1; j >= 0; j--) {
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

bool Query::is_allowed_solution(const int context_depth, const FunctionRef &solution) {
    // A solution is allowed only if it does not depend on contexts deeper than context_depth
    const Context *context = m_context;
    int depth = m_context_depth;
    while (depth > context_depth) {
        if (solution->depends_on(context->functions()))
            return false;

        context = context->parent();
        --depth;
    }
    // TODO: solution is also not allowed to depend on things from telescope that have a depth larger than context_depth !
    return true;
}
