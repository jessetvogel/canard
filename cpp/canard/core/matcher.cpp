//
// Created by Jesse Vogel on 01/07/2021.
//

#include "matcher.h"
#include "specialization.h"

#include <utility>
#include <iostream>
#include <sstream>

FunctionPtr Matcher::null = nullptr;

Matcher::Matcher(const std::vector<FunctionPtr> &indeterminates) : m_parent(nullptr),
                                                                   m_indeterminates(indeterminates) {}

Matcher::Matcher(Matcher *parent, const std::vector<FunctionPtr> &indeterminates) : m_parent(parent),
                                                                                    m_indeterminates(indeterminates) {}

bool Matcher::put_solution(const FunctionPtr &f, const FunctionPtr &g) {
    // Case f = g: we are not going to map f -> f, but we will return true nonetheless
    if (f == g) return true;

    // Case g -> h: we map f -> h as well
    const FunctionPtr &h = get_solution(g);
    if (h != nullptr)
        return put_solution(f, h);

    // If f -> k already, do some checks
    // TODO: we must prevent loops "f -> g -> f"
    // NOTE: it is important we only look for solutions by THIS matcher, and not of any parent. This is because sub_matchers may overwrite certain indeterminates
    auto it_solution_f = m_solutions.find(f);
    if (it_solution_f != m_solutions.end()) {
        const FunctionPtr &k = it_solution_f->second;
        if (k->equals(g)) return true;

        // If k is also an indeterminate (possibly of a parent!), then we are satisfied with mapping g -> k
        if (is_indeterminate(k)) return put_solution(g, k);

        // Finally, if g is an indeterminate, it was apparently not mapped before. Hence we will map g -> f instead
        if (is_indeterminate(g)) return put_solution(g, f);

//        std::cerr << "cannot match " << f->to_string() << " to " << g->to_string() << " because already matched to " << k->to_string() << std::endl;
        return false;
    }

    // If all is well, put f -> g.
    m_solutions.emplace(f, g);
    return true;
}

const FunctionPtr &Matcher::get_solution(const FunctionPtr &f) const {
    auto it_f = m_solutions.find(f);
    if (it_f == m_solutions.end())
        return (m_parent == nullptr) ? null : m_parent->get_solution(f);

    // Case f -> g -> h
    const FunctionPtr &h = get_solution(it_f->second);
    return (h == nullptr) ? it_f->second : h;
}

bool Matcher::is_indeterminate(const FunctionPtr &f) {
    for (Matcher *matcher = this; matcher != nullptr; matcher = matcher->m_parent) {
        auto &indeterminates = matcher->m_indeterminates;
        if (std::find(indeterminates.begin(), indeterminates.end(), f) != indeterminates.end())
            return true;
    }
    return false;
}

bool Matcher::matches(const FunctionPtr &f, const FunctionPtr &g) {
    // If f equals g, there is obviously a match, and nothing more to do
    if (f == g) return true;

    // Number of dependencies should agree
    auto &f_dependencies = f->get_dependencies(), &g_dependencies = g->get_dependencies();
    size_t n = f_dependencies.size();
    if (n != g_dependencies.size())
        return false;

    // Dependencies themselves should match
    auto sub_matcher = (n > 0) ? std::make_unique<Matcher>(this, f_dependencies.m_functions) : nullptr;
    Matcher &use_matcher = (n > 0) ? *sub_matcher : *this;
    for (int i = 0; i < n; ++i) {
        // Explicitness must match
        if (f_dependencies.m_explicits[i] != g_dependencies.m_explicits[i])
            return false;
        // Functions must match
        if (!use_matcher.matches(f_dependencies.m_functions[i], g_dependencies.m_functions[i]))
            return false;
    }

    // Types should match (note: up to the matches already made by use_matcher)
    if (!use_matcher.matches(f->get_type(), g->get_type()))
        return false;

    // At this point, f and g agree up to their signature, i.e. their dependencies and types match

    // If f (resp. g) is an indeterminate, map f -> g (resp. g -> f).
    // Also treat the case where f or g is an indeterminate of some parent
    for (Matcher *m = this; m != nullptr; m = m->m_parent) {
        if (std::find(m->m_indeterminates.begin(), m->m_indeterminates.end(), f) != m->m_indeterminates.end())
            return m->put_solution(f, g);
        if (std::find(m->m_indeterminates.begin(), m->m_indeterminates.end(), g) != m->m_indeterminates.end())
            return m->put_solution(g, f);
    }

    // If the bases match, simply match the arguments
    // Note that the bases themselves can be indeterminates, so we have to account for that first.
    // Of course it can also be possible that fBase == gBase (this can be checked as Java Objects, since no base function
    // is represented by different Java Objects)
    const FunctionPtr &f_base = f->get_base();
    const FunctionPtr &g_base = g->get_base();

    // TODO: what if fBase/gBase is an indeterminate, but also it has dependencies ?
    //  At some points problems appear, because there will be multiple solutions..
    if (f_base == g_base || ((is_indeterminate(f_base) || is_indeterminate(g_base)) && matches(f_base, g_base))) {
        auto &f_arguments = f->get_arguments(), &g_arguments = g->get_arguments();
        size_t m = f_arguments.size(); // Since the bases already match, we know the number of arguments must agree as well
        for (int i = 0; i < m; ++i) {
            if (!matches(f_arguments[i], g_arguments[i]))
                return false;
        }
        return true;
    }
    return false;
}

void Matcher::assert_matches(const FunctionPtr &f, const FunctionPtr &g) {
    if (!matches(f, g)) {
        std::cerr << "Failed assertion: matching " << f->to_string(true, false) << " to " << g->to_string(true, false)
                  << "!" << std::endl;
        assert(false);
    }
}

FunctionPtr Matcher::convert(const FunctionPtr &f) {
    // If f -> g, return g
    const FunctionPtr &g = get_solution(f);
    if (g != nullptr) return g;

    FunctionPtr f_base = f->get_base();
    if (f == f_base) // Note that this is actually a special case of the next step!
        return f;

    // Convert dependencies if needed
    // E.g. something like "(f : Hom X X) => comp f f" must be converted under "X -> Spec R"
    // to "(f : Hom (Spec R) (Spec R)) => comp f f"
    Function::Dependencies converted_dependencies;
    const Function::Dependencies &f_dependencies = f->get_dependencies();
    size_t n = f_dependencies.size();

    Matcher matcher(this, f_dependencies.m_functions);
    Matcher &sub_matcher = (n > 0) ? matcher : *this;
    for (int i = 0; i < n; ++i) {
        auto &x = f_dependencies.m_functions[i];
        FunctionPtr clone = sub_matcher.clone(x);
        sub_matcher.assert_matches(x, clone);

        converted_dependencies.m_functions.push_back(clone);
        converted_dependencies.m_explicits.push_back(f_dependencies.m_explicits[i]);
    }

    // Otherwise, when f is a specialization, we convert each argument of f
    bool changes = false;
    std::vector<FunctionPtr> converted_arguments;
    for (auto &arg : f->get_arguments()) {
        FunctionPtr converted_argument = sub_matcher.convert(arg);
        if (!changes && !converted_argument->equals(arg))
            changes = true;
        converted_arguments.push_back(converted_argument);
    }

    // Note: also possibly the base should be converted!
    FunctionPtr f_base_solution = get_solution(f_base);
    if (f_base_solution != nullptr && f_base_solution != f_base) {
        f_base = f_base_solution; // (silently change the base)
        changes = true;
    }

    // If no actual changes were made to the arguments or the base, we can simply return the original f
    if (!changes)
        return f;

    // Now we must convert the type of f, this requires another subMatcher!
    // Because the type is determined by the arguments provided to the base Function,
    // since we replace those now, the type should be re-determined
//    auto& base_dependencies = f_base->get_dependencies().m_functions;
//    List <Function> baseExplicitDependencies = base.getExplicitDependencies();
    auto &f_base_dependencies = f_base->get_dependencies();
    Matcher sub_sub_matcher(&sub_matcher, f_base_dependencies.m_functions);
    size_t m = f_base_dependencies.size();
    int j = 0;
    for (int i = 0; i < m; ++i) {
        if (!f_base_dependencies.m_explicits[i])
            continue;

        sub_sub_matcher.assert_matches(f_base_dependencies.m_functions[i], converted_arguments[j]);
        j++;
    }
    FunctionPtr converted_type = sub_sub_matcher.convert(f->get_type());

    return std::make_shared<Specialization>(f_base, std::move(converted_arguments), std::move(converted_type), std::move(converted_dependencies));
}

FunctionPtr Matcher::clone(const FunctionPtr &f) {
    // Since we only use this for duplicating dependencies, and dependencies are always their own base function!
    assert (f == f->get_base());
    // TODO: does it make sense to duplicate a specialization ?
    // TODO: are there any shortcuts we can take so that we can simply immediately return x itself ?

    // Duplicate *its* dependencies
    Function::Dependencies converted_dependencies;
    auto &f_dependencies = f->get_dependencies();
    size_t n = f_dependencies.size();

    Matcher matcher(this, f_dependencies.m_functions);
    Matcher &sub_matcher = (n > 0) ? matcher : *this;

    for (int i = 0; i < n; ++i) {
        auto clone = sub_matcher.clone(f_dependencies.m_functions[i]);
        sub_matcher.assert_matches(f_dependencies.m_functions[i], clone);
        converted_dependencies.m_functions.push_back(clone);
    }
    converted_dependencies.m_explicits = f_dependencies.m_explicits;

    // Convert the type (according to the subMatcher!)
    auto converted_type = sub_matcher.convert(f->get_type());

    // Create a new Function
    auto g = std::make_shared<Function>(converted_type, std::move(converted_dependencies));
    g->set_label(f->get_label());
    return g;
}

FunctionPtr Matcher::cheap_clone(const FunctionPtr &f) {
    // Should return x itself when x does not depend on any indeterminate (and neither on an indeterminate of some parent)
    for (Matcher *m = this; m != nullptr; m = m->m_parent) {
        if (f->signature_depends_on(m->m_indeterminates))
            return m->clone(f); // TODO: m->clone(f) ?
    }
    return f;
}

std::string Matcher::to_string() {
    std::stringstream ss;
    ss << "{[";
    bool first = true;
    for (auto &f : m_indeterminates) {
        if (!first)
            ss << ", ";
        first = false;
        ss << f->to_string();
    }
    ss << "] ";

    first = true;
    for (auto &m_solution : m_solutions) {
        if (!first)
            ss << ", ";
        first = false;
        ss << m_solution.first->to_string() << " -> " << m_solution.second->to_string();
    }
    ss << '}';
    return ss.str();
}
