//
// Created by Jesse Vogel on 01/07/2021.
//

#include "Query.h"
#include "../core/macros.h"
#include "../core/Formatter.h"
#include <utility>
#include <sstream>
#include <algorithm>

Query::Query(std::vector<FunctionPtr> indeterminates) : m_parent(nullptr), m_indeterminates(std::move(indeterminates)) {
    m_depths.insert(m_depths.begin(), m_indeterminates.size(), 0); // initial depth is 0
}

Query::Query(std::shared_ptr<Query> parent, std::unordered_map<FunctionPtr,
        FunctionPtr> solutions, std::vector<FunctionPtr> indeterminates,
             std::vector<int> depths) : m_parent(std::move(parent)),
                                        m_indeterminates(std::move(indeterminates)),
                                        m_depths(std::move(depths)) {
    m_solutions = std::move(solutions);
}

int Query::depth() {
    int M = 0;
    for (int &d: m_depths)
        if (d > M)
            M = d;
    return M;
}

bool Query::is_indeterminate(const FunctionPtr &f) {
    return std::find(m_indeterminates.begin(), m_indeterminates.end(), f) != m_indeterminates.end();
}

std::shared_ptr<Query> Query::normalize(const std::shared_ptr<Query> &query) {
    // Get last indeterminate
    const FunctionPtr &h = query->last_indeterminate();

    // If h has no dependencies, nothing to do here
    if (h->dependencies().size() == 0)
        return query;

    // Create a sub_query where h is replaced by a dependency-less version of h
    // This introduces new locals: the current dependencies of h
    std::vector<FunctionPtr> new_indeterminates = query->m_indeterminates;
    auto new_h = std::make_shared<Function>(h->type(), Function::Dependencies());
    new_indeterminates.back() = new_h;
    std::unordered_map<FunctionPtr, FunctionPtr> new_solutions;
    new_solutions.emplace(h, new_h);

    // Just use the same depths, doesn't seem fair to charge for a normalize-step
    std::shared_ptr<Query> sub_query(new Query(
            query,
            std::move(new_solutions),
            std::move(new_indeterminates),
            query->m_depths));

    sub_query->m_locals = query->m_locals;
    sub_query->m_locals_allowed = query->m_locals_allowed;

    // Add the dependencies of h to the locals of sub_query
    // Then hNew may depend on these new locals
    auto it = query->m_locals_allowed.find(h);
    std::shared_ptr<std::unordered_set<FunctionPtr>> new_h_allowed =
            (it != query->m_locals_allowed.end())
            ? std::make_shared<std::unordered_set<FunctionPtr>>(*(it->second)) // add all that h was allowed to
            : std::make_shared<std::unordered_set<FunctionPtr>>();

    for (auto &f: h->dependencies().m_functions) {
        sub_query->m_locals.push_back(f);
        new_h_allowed->insert(f);
    }

    sub_query->m_locals_allowed.erase(h);
    sub_query->m_locals_allowed.emplace(new_h, new_h_allowed);

    return sub_query;
}

std::shared_ptr<Query> Query::reduce(const std::shared_ptr<Query> &query, const FunctionPtr &thm) {
    // Get last indeterminate
    const FunctionPtr &h = query->last_indeterminate();

    // At this point, h is asserted to have no dependencies!
    assert (h->dependencies().empty());

    // Get allowed locals for h
    auto it_h_allowed_locals = query->m_locals_allowed.find(h);
    bool h_has_allowed_locals = (it_h_allowed_locals != query->m_locals_allowed.end());

    // Check: if thm *itself* is a local: then require that it_thm is allowed for h! This is very important.
    auto it_thm = std::find(query->m_locals.begin(), query->m_locals.end(), thm);
    if (it_thm != query->m_locals.end() &&
        (!h_has_allowed_locals || it_h_allowed_locals->second->find(thm) == it_h_allowed_locals->second->end()))
        return nullptr;

    // Create matcher with indeterminates from both query and thm
    std::vector<FunctionPtr> all_indeterminates = query->m_indeterminates;
    const auto &thm_dependencies = thm->dependencies().m_functions;
    all_indeterminates.insert(all_indeterminates.end(), thm_dependencies.begin(), thm_dependencies.end());
    Matcher matcher(all_indeterminates);

    // Note that we do not match (h, thm) since they might not match!
    // (E.g. when thm has dependencies that require arguments, while h has not, there will be no match for h and thm, but still
    // thm could be applied to get a solution for h)
    // (Also, e.g. when h has dependencies, but we are constructing lambda-style)
    // Therefore we will just match the types of h and thm
    if (!matcher.matches(h->type(), thm->type()))
        return nullptr;

    CANARD_DEBUG("Valid reduction in [" << query->to_string() << "]");
    CANARD_DEBUG("=> Solve for " << h->to_string() << " with " << thm->to_string());

    // Create the new sub_query, and keep track of the solutions (for indeterminates) and the arguments (for thm)
    // and the new indeterminates (for the sub_query)
    std::vector<FunctionPtr> new_indeterminates;
    std::unordered_map<FunctionPtr, FunctionPtr> new_solutions, arguments;
    std::vector<int> new_depths;
    std::vector<FunctionPtr> new_locals;
    std::unordered_map<FunctionPtr, std::shared_ptr<std::unordered_set<FunctionPtr>>> new_locals_allowed;

    const int h_depth = query->m_depths.back();

    std::vector<FunctionPtr> mappable = all_indeterminates;

    // The motivation here is that NOTHING should depend on h, as it is the last indeterminate.
    // It might happen that some local variable is depends on h, but in that case we should just throw it away
    // (because it cannot be that another indeterminate will depend on that local variable, as it would then in turn
    // depend on h again). So in conclusion, we can just forget about h for now, and then in the end construct
    // the value/solution for h!
    mappable.erase(std::find(mappable.begin(), mappable.end(), h));
    mappable.insert(mappable.end(), query->m_locals.begin(), query->m_locals.end()); // locals might need mapping too!

    Matcher query_to_sub_query(mappable);

    std::vector<FunctionPtr> unmapped = mappable;
    bool changes = true;
    while (changes && !unmapped.empty()) {
        changes = false;
        for (auto it = unmapped.begin(); it != unmapped.end(); ++it) {
            const auto f = *it;
            // When f is an indeterminate of this query
            auto it_f = std::find(query->m_indeterminates.begin(), query->m_indeterminates.end(), f);
            if (it_f != query->m_indeterminates.end()) {
                auto solution = matcher.get_solution(f);
                if (solution != nullptr) {
                    // If f has a solution, the solution must not depend on any unmapped indeterminate
                    // And, in this case, we simply convert along the new matcher
                    if (solution->depends_on(unmapped))
                        continue;

                    // Check whether `solution` is allowed (w.r.t. the local variables)
                    if (!query->is_allowed_solution(f, solution)) {
                        CANARD_DEBUG("Nope, " << solution->to_string() << " is not an allowed solution for "
                                              << f->to_string(true, false));
                        return nullptr;
                    }

                    solution = query_to_sub_query.convert(solution);
                } else {
                    // If f has no solution, this indeterminate remains an indeterminate.
                    // Hence, we duplicate it_thm to the new query
                    if (f->signature_depends_on(unmapped))
                        continue;

                    // This can be done cheaply, i.e. might preserve f as an indeterminate
                    solution = query_to_sub_query.cheap_clone(f);
                    if (solution != f)
                        solution->set_label(f->label()); // not really needed, but pretty for debugging

                    new_indeterminates.push_back(solution);

                    size_t i = it_f - query->m_indeterminates.begin();
                    // the indeterminate has not changed, so its depth remains the same
                    new_depths.push_back(query->m_depths[i]);

                    // Pass on the allowed locals (if f has any) (Note: the locals will get replaced later on)
                    auto it_f_allowed = query->m_locals_allowed.find(f);
                    if (it_f_allowed != query->m_locals_allowed.end())
                        new_locals_allowed.emplace(solution, it_f_allowed->second);
                }
                // Store the solution, un-mark as unmapped, match, and indicate that changes are made
                if (solution != f)
                    new_solutions.emplace(f, solution);

                query_to_sub_query.assert_matches(f, solution);
                it = unmapped.erase(it);
                --it;
                changes = true;
                break;
            }
                // Case it_thm is a dependency of thm:
            else if (std::find(thm_dependencies.begin(), thm_dependencies.end(), f) != thm_dependencies.end()) {
                auto argument = matcher.get_solution(f);
                if (argument != nullptr) {
                    // If the argument is set, the argument must not depend on unmapped stuff
                    // And, in that case, convert along the new matcher
                    if (argument->depends_on(unmapped))
                        continue;

                    // Check whether `argument` is allowed (w.r.t. the local variables)
                    // Note that it has the same allowances as h, because it is used to solve for h
                    if (h_has_allowed_locals && !query->is_allowed_solution(*it_h_allowed_locals->second, argument)) {
                        CANARD_DEBUG("Nope, " << argument->to_string() << " is not an allowed argument for "
                                              << f->to_string(true, false));
                        return nullptr;
                    }

                    argument = query_to_sub_query.convert(argument);
                } else {
                    // If no argument is set, it_thm becomes a new indeterminate
                    // Hence we duplicate it_thm to the new query
                    if (f->signature_depends_on(unmapped))
                        continue;
                    // Note: no cheap clone here! This might cause confusion e.g. when theorems are applied twice
                    argument = query_to_sub_query.clone(f);
                    if (argument != f)
                        argument->set_label(f->label()); // not really needed, but pretty for debugging

                    new_indeterminates.push_back(argument);
                    new_depths.push_back(h_depth + 1); // indeterminate was introduced because of h, so h_depth + 1

                    // This new indeterminate may depend on the local variables that h was allowed to depend on
                    if (h_has_allowed_locals && !it_h_allowed_locals->second->empty())
                        // (Note: the locals will get replaced later on)
                        new_locals_allowed.emplace(argument, it_h_allowed_locals->second);
                }
                // Store the argument, un-mark as unmapped, match, and indicate that changes are made
                CANARD_ASSERT(argument != f, "Argument cannot be f");
                arguments.emplace(f, argument);
                query_to_sub_query.assert_matches(f, argument);
                it = unmapped.erase(it);
                --it;
                changes = true;
                break;
            }
                // Case f is a local
            else if (std::find(query->m_locals.begin(), query->m_locals.end(), f) != query->m_locals.end()) {
//            else {
                // If the signature of this local depends on something unmapped, we cannot know yet what to do
                if (f->signature_depends_on(unmapped))
                    continue;

                // If the local variable depends on h, we simply throw it away (for justification see above)
                // TODO: make this cleaner ?
                if (f->signature_depends_on({h})) {
                    it = unmapped.erase(it);
                    --it;
                    changes = true;
                    CANARD_DEBUG("Throwing away local " << f->to_string(true, false) << " because it depends on h!");
                    break;
                }

                // We should clone this dependency (cheaply!) and add it_thm as a local of sub_query
                FunctionPtr f_clone = query_to_sub_query.cheap_clone(f);
                // Note: we do not yet push this local to new_locals, because it might be that none of the indeterminates actually are allowed to use it.
                // In that case it makes more sense to just omit the local. Hence, later, when converting the locals (something that needs to be done anyway)
                // we will set new_locals
                new_locals.push_back(f_clone);
                if (f_clone != f) // if something changed, then add as a solution
                    new_solutions.emplace(f, f_clone);
                query_to_sub_query.assert_matches(f, f_clone);
                it = unmapped.erase(it);
                --it;
                changes = true;
                break;
            }

            CANARD_ASSERT(false, "Unmapped function which is neither indeterminate, thm dependency nor local variable");
        }
    }

    // If there are still unmapped indeterminates, there must be some dependency-loop, so we concede and say
    // we cannot reduce to a sub_query
    if (!unmapped.empty()) {
        CANARD_DEBUG("Circular dependence!");
        return nullptr;
    }

    // Now, sub_query.locals is set.
    // Also sub_query.locals_allowed is set, but with the OLD locals. We simply need to replace those!
    // Meanwhile, we set new_locals here, because here we know which locals actually get used
    for (auto &it: new_locals_allowed) {
        auto converted_set = std::make_shared<std::unordered_set<FunctionPtr>>();
        for (const FunctionPtr &f: *it.second) {
            auto converted_local = query_to_sub_query.convert(f);
            converted_set->insert(converted_local);
//            new_locals.insert(converted_local); // We add this to new_locals, because apparently it gets used!
        }
        it.second = converted_set;
    }

    // Finally, set solution of h, as specialization of thm
    std::vector<FunctionPtr> thm_arguments = thm->explicit_dependencies();
    for (FunctionPtr &f: thm_arguments)
        f = arguments[f];

    try {
        // Note that thm itself must also be converted, since it_thm might have changed along the way to sub_query!
        // E.g. for `search (P {A B : Prop} (h (a : A) : B) (x : A) : B)`
        new_solutions.emplace(h, query_to_sub_query.convert(thm)->specialize(std::move(thm_arguments), {}));
    } catch (SpecializationException &e) {
        CANARD_ASSERT(false, "Ai ai ai! Not what was supposed to happen!");
    }

    // Create the reduced query based on the solutions and new indeterminates
    std::shared_ptr<Query> sub_query(new Query(query, new_solutions, new_indeterminates, new_depths));
    sub_query->m_locals = std::move(new_locals);
    sub_query->m_locals_allowed = std::move(new_locals_allowed);
    return sub_query;
}

std::vector<FunctionPtr> Query::final_solutions(const std::shared_ptr<Query> &query) {
    // This can only be done if this query is solved
    CANARD_ASSERT(query->is_solved(), "Cannot call final_solutions on unsolved query");

    // Create chain of sub-queries, starting at the bottom, all the way up to the top
    // Note: we include every query which has a parent, so the initial query will not be included
    std::vector<std::shared_ptr<Query>> chain;
    for (auto q = query; q->parent() != nullptr; q = q->parent())
        chain.push_back(q);

    /* The idea is as follows: starting at the back, `matcher` will map from Query ..., 4, 3, 2, 1 to the solution,
       at each step using the previous matcher

        Query 1 -> Query 2 -> Query 3 -> Query 4
            \           \          \       |
             ----->      ----->     -> Solution
    */

    const size_t n = chain.size();
    std::vector<std::unique_ptr<Matcher>> matchers;
    std::vector<std::unique_ptr<std::vector<FunctionPtr>>> matchers_indeterminates;
    for (int i = 0; i < n; ++i) {
        auto &q = chain[i];
        CANARD_ASSERT(q->parent() != nullptr,
                      "Chain should only contain queries which have a parent"); // (just to be sure)

        // Obtain key set
        auto indeterminates = std::unique_ptr<std::vector<FunctionPtr>>(new std::vector<FunctionPtr>());
        for (auto &it: q->m_solutions)
            indeterminates->push_back(it.first);
        auto next_matcher = (i == 0)
                            ? std::unique_ptr<Matcher>(new Matcher(*indeterminates))
                            : std::unique_ptr<Matcher>(new Matcher(matchers.back().get(), *indeterminates));
        matchers_indeterminates.push_back(std::move(indeterminates));

        auto h = q->parent()->last_indeterminate();
        for (auto &it: q->m_solutions) { // TODO: reason is that locals can also be mapped to solutions, and potentially not all indeterminates need to have solutions at each step ?
            auto f = it.first;
            auto g = it.second;

            // First convert the solution along the previous matcher
            if (i > 0) {
                bool is_local = (std::find(q->parent()->m_locals.begin(), q->parent()->m_locals.end(), it.first) !=
                                 q->parent()->m_locals.end());
                if (is_local)
                    // locals must be cloned, because their type may depend on indeterminates, while their expression may not
                    g = matchers.back()->cheap_clone(g);
                else
                    g = matchers.back()->convert(g); // non-locals can simply be converted
            }

            // Case f = h, may need to convert the dependencies
            if (f == h) {
                const size_t m = h->dependencies().size();
                if (m > 0) {
                    try {
                        Function::Dependencies new_dependencies;
                        if (i == 0)
                            new_dependencies = h->dependencies();
                        else {
                            new_dependencies.m_explicits = h->dependencies().m_explicits;
                            for (int j = 0; j < m; ++j)
                                new_dependencies.m_functions.push_back(
                                        matchers.back()->convert(h->dependencies().m_functions[j]));
                        }
                        g = g->specialize({}, std::move(new_dependencies));
                    }
                    catch (SpecializationException &e) {
                        CANARD_ASSERT(false, e.m_message);
                    }
                }
            }

            // Now the next matcher should map f to g
            next_matcher->assert_matches(f, g);
        }

        // Add to chain of matchers
        matchers.push_back(std::move(next_matcher));
    }

    // Finally, we can convert the indeterminates of the initial query
    // Note: this is the parent of the last query in the chain (because the initial query does not have a parent,
    // so it is not in the chain)
    std::vector<FunctionPtr> result;
    for (auto &f: chain.back()->parent()->m_indeterminates)
        result.push_back(matchers.back()->convert(f));
    return result;
}

bool Query::injects_into(const std::shared_ptr<Query> &other) {
    assert(other != nullptr);
    // Goal is to map (injectively) all my indeterminates to other.indeterminates, but all other.locals to my locals
    // That is, we are checking if this query searches for less with more information.

    // Probably start at the end, work way towards the beginning
    // Method should be recursive
    // Can we just use Matchers ?

    // 1. let f = indeterminates[-1]
    // 2. for each g in other.indeterminates:
    //   2.1. Create sub_matcher with f -> g. If fail, continue with next g
    //   2.2. If succeeded, see what indeterminates are mapped, and update the lists of which to map and which are still allowed from other
    //   2.3. Continue recursively to the next (not yet mapped (also not induced mapped)) indeterminate (from the end, remember)

    // At any point, we need:
    // - current matcher (possibly null)
    // - indeterminates yet to be mapped
    // - allowed other.indeterminates to map to

//        System.out.println("Does [" + this + "] inject into [" + other + "] ? ");

    // Shortcut: if this is 'bigger' than other, it won't work
    if (m_indeterminates.size() > other->m_indeterminates.size())
        return false;

    return injects_into_helper(nullptr, m_indeterminates, other->m_indeterminates);

    // TODO: actually, we would probably also need to check for the local variables..
}

bool Query::injects_into_helper(Matcher *matcher, std::vector<FunctionPtr> unmapped, std::vector<FunctionPtr> allowed) {
    // Base case
    size_t n = unmapped.size();
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
    int m = (int) allowed.size();
    for (int j = m - 1; j >= 0; j--) {
        auto &g = allowed[j];
        bool valid_candidate = true;

        Matcher sub_matcher = (matcher == nullptr) ? Matcher(unmapped) : Matcher(matcher, unmapped);
        if (!sub_matcher.matches(f, g))
            continue;

        // Create new unmapped and allowed lists, by removing those which are mapped and to which they are mapped
        std::vector<FunctionPtr> new_unmapped, new_allowed = allowed;
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

bool Query::is_allowed_solution(const FunctionPtr &f, const FunctionPtr &solution) {
    if (m_locals.empty()) return true;

    auto it_allowed = m_locals_allowed.find(f);
    if (it_allowed != m_locals_allowed.end())
        return is_allowed_solution(*it_allowed->second, solution);
    return is_allowed_solution(std::unordered_set<FunctionPtr>(), solution);
}

bool Query::is_allowed_solution(const std::unordered_set<FunctionPtr> &allowed, const FunctionPtr &solution) {
    if (m_locals.empty()) return true;

    std::vector<FunctionPtr> disallowed;
    for (const FunctionPtr &l: m_locals) {
        if (allowed.find(l) == allowed.end())
            disallowed.push_back(l);
    }
    return disallowed.empty() || !solution->depends_on(disallowed);
}

std::string Query::to_string() {
    std::ostringstream ss;
    ss << "Query@" << depth() << " {";
    for (auto &f: m_locals)
        ss << " (" << Formatter::to_string(f, true, false) << ")";
    ss << " } ";
    for (auto &f: m_indeterminates)
        ss << " (" << Formatter::to_string(f, true, false) << ")";
    return ss.str();
}

bool Query::has_parent(const std::shared_ptr<Query> &p) const {
    for (std::shared_ptr<Query> r = parent(); r != nullptr; r = r->parent())
        if (r == p)
            return true;
    return false;
}