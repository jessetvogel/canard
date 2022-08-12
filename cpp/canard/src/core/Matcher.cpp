//
// Created by Jesse Vogel on 01/07/2021.
//

#include "Matcher.h"
#include "macros.h"

#include <utility>
#include <algorithm>
#include <memory>

Matcher::Matcher(const std::vector<FunctionRef> &indeterminates) : m_parent(nullptr),
                                                                   m_indeterminates(indeterminates) {}

Matcher::Matcher(Matcher *parent, const std::vector<FunctionRef> &indeterminates) : m_parent(parent),
                                                                                    m_indeterminates(indeterminates) {}

bool Matcher::put_solution(const FunctionRef &f, const FunctionRef &g) {
    // Case f = g: we are not going to map f -> f (causes infinite loops), but we will return true nonetheless
    if (f == g) return true;

    // Case g -> h: we map f -> h as well
    const auto &h = get_solution(g);
    if (h != nullptr) return put_solution(f, h);

    // If f -> k already, do some checks
    // TODO: we must prevent loops "f -> g -> f"
    // NOTE: it is important we only look for solutions by THIS matcher, and not of any parent. This is because sub_matchers may overwrite certain indeterminates
    auto it_solution_f = m_solutions.find(f);
    if (it_solution_f != m_solutions.end()) {
        const auto &k = it_solution_f->second;
        if (k.equivalent(g)) return true;

        // TODO: the order in which to put things: either g -> k or k -> g I'm not so sure about.
        //  we want to keep some order: only map 'from left to right', so should we check whether k < g or k > g ?

        // If g is an indeterminate, it was apparently not mapped before (because we checked for that). Hence, we will map g -> f -> k instead
        if (is_indeterminate(g)) return put_solution(g, k); // Since f -> k, we might as well map g -> k directly!

        // If k is also an indeterminate (possibly of a parent!), then we are satisfied with mapping k -> g
        if (is_indeterminate(k)) return put_solution(k, g);

        // In case g and k are both *not* indeterminates, they may still match! Let's check this:
        return matches(k, g);
    }

    m_solutions.emplace(f, g);
    return true;
}

const FunctionRef &Matcher::get_solution(const FunctionRef &f) const {
    // Look for solution for f, it not found, ask parent
    auto it_solution_f = m_solutions.find(f);
    if (it_solution_f == m_solutions.end())
        return (m_parent == nullptr) ? FunctionRef::null() : m_parent->get_solution(f);

    // To deal with cases like f -> g -> h, call get_solution recursively
    const auto &h = get_solution(it_solution_f->second);
    return (h == nullptr) ? it_solution_f->second : h;
}

bool Matcher::is_indeterminate(const FunctionRef &f) {
    // Checks whether f is an indeterminate of this or some parent
    for (Matcher *matcher = this; matcher != nullptr; matcher = matcher->m_parent) {
        const auto &indeterminates = matcher->m_indeterminates;
        if (std::find(indeterminates.begin(), indeterminates.end(), f) != indeterminates.end())
            return true;
    }
    return false;
}

bool Matcher::matches(const FunctionRef &f, const FunctionRef &g) {
    // TODO: at some point, implement that two proofs of the same proposition should match

    // If f equals g, there is obviously a match, and nothing more to do
    if (f == g) return true;

    // Number of parameters should agree
    auto &f_parameters = f->parameters(), &g_parameters = g->parameters();
    size_t n = f_parameters.size();
    if (n != g_parameters.size())
        return false;

    // Parameters themselves should match
    auto sub_matcher = (n > 0) ? std::unique_ptr<Matcher>(new Matcher(this, f_parameters.functions())) : nullptr;
    Matcher &use_matcher = (n > 0) ? *sub_matcher : *this;
    for (int i = 0; i < n; ++i) {
        // Explicitness must match // TODO: should it?
//        if (f_parameters.m_explicits[i] != g_parameters.m_explicits[i])
//            return false;
        // Functions must match
        if (!use_matcher.matches(f_parameters.functions()[i], g_parameters.functions()[i]))
            return false;
    }

    // Types should match (note: up to the matches already made by use_matcher)
    if (!use_matcher.matches(f.type(), g.type()))
        return false;

    // At this point, f and g agree up to their signature, i.e. their parameters and types match

    // If f (resp. g) is an indeterminate, map f -> g (resp. g -> f).
    // Also treat the case where f or g is an indeterminate of some parent
    for (Matcher *m = this; m != nullptr; m = m->m_parent) {
        auto it_f = std::find(m->m_indeterminates.begin(), m->m_indeterminates.end(), f);
        auto it_g = std::find(m->m_indeterminates.begin(), m->m_indeterminates.end(), g);
        if (it_f != m->m_indeterminates.end() || it_g != m->m_indeterminates.end()) {
            // If both f and g are indeterminates, we preferably map 'from left to right' in the list of indeterminates,
            // to have some consistency at least
            return (it_f < it_g) ? m->put_solution(f, g) : m->put_solution(g, f);
        }
    }

    // If the bases match, simply match the arguments
    // Note that the bases themselves can be indeterminates, so we have to account for that first.
    const auto &f_base = f.base();
    const auto &g_base = g.base();

    // TODO: what if fBase/gBase is an indeterminate, but also it contains parameters ?
    //  At some points problems appear, because there will be multiple solutions..
    if (f_base == g_base || ((is_indeterminate(f_base) || is_indeterminate(g_base)) && matches(f_base, g_base))) {
        auto &f_arguments = f->arguments(), &g_arguments = g->arguments();
        size_t m = f_arguments.size(); // Since the bases already match, we know the number of arguments must agree as well
        for (int i = 0; i < m; ++i) {
            if (!matches(f_arguments[i], g_arguments[i]))
                return false;
        }
        return true;
    }

//    std::cerr << f->to_string() << " does not match " << g->to_string() << std::endl;
    return false;
}

void Matcher::assert_matches(const FunctionRef &f, const FunctionRef &g) {
    bool match = matches(f, g);
    CANARD_ASSERT(match, "Matching  << Formatter::to_string(f, true, false) <<  to << Formatter::to_string(g, true, false)");
}

FunctionRef Matcher::convert(const FunctionRef &f) {
    // If f -> g, return g
    const FunctionRef &g = get_solution(f);
    if (g != nullptr) return g;

    // If f is a base function (and contains no solution), then the conversion can only be f itself
    if (f->is_base()) return f; // (Note that this is actually a special case of the next step!)

    // Convert parameters if needed
    // E.g. something like "λ (f : Hom X X) := comp f f" must be converted under "X -> Spec R"
    // to "λ (f : Hom (Spec R) (Spec R)) := comp f f"
    Telescope converted_parameters;
    const Telescope &f_parameters = f->parameters();
    const size_t n = f_parameters.size();

    std::unique_ptr<Matcher> matcher = (n > 0)
                                       ? std::unique_ptr<Matcher>(new Matcher(this, f_parameters.functions()))
                                       : nullptr;
    Matcher &sub_matcher = (n > 0) ? *matcher : *this;
    for (int i = 0; i < n; ++i) {
        const auto &x = f_parameters.functions()[i];
        auto clone = sub_matcher.clone(x);
        sub_matcher.assert_matches(x, clone);

        converted_parameters.add(std::move(clone));
//        converted_parameters.m_explicits.push_back(f_parameters.m_explicits[i]);
    }

    // Otherwise, when f is a specialization, we convert each argument of f
    bool changes = false;
    std::vector<FunctionRef> converted_arguments;
    for (auto &arg: f->arguments()) {
        auto converted_argument = sub_matcher.convert(arg);
        if (!changes && !converted_argument.equivalent(arg))
            changes = true;
        converted_arguments.push_back(std::move(converted_argument));
    }

    // Note: also possibly the base should be converted!
    const FunctionRef &f_base = f.base();
    const FunctionRef &f_base_solution = get_solution(f_base);
    bool f_base_changed = (f_base_solution != nullptr && f_base_solution != f_base);
    const FunctionRef &f_base_new = f_base_changed ? f_base_solution : f_base;
    changes |= f_base_changed;

    // If no actual changes were made to the arguments or the base, we can simply return the original f
    if (!changes) return f;

    // Now we must convert the type of f, this requires another sub_matcher!
    // Because the type is determined by the arguments provided to the base Function,
    // since we replace those now, the type should be re-determined
    auto &f_base_parameters = f_base_new->parameters();
    Matcher sub_sub_matcher(&sub_matcher, f_base_parameters.functions());
    const size_t m = f_base_parameters.size();
    int j = 0;
    for (int i = 0; i < m; ++i)
        sub_sub_matcher.assert_matches(f_base_parameters.functions()[i], converted_arguments[j++]);

    return Function::make(
            sub_sub_matcher.convert(f.type()), std::move(converted_parameters),
            f_base_new, std::move(converted_arguments)
    );
}

FunctionRef Matcher::clone(const FunctionRef &f) {
    // Clone the parameters
    Telescope cloned_parameters;
    const auto &parameters = f->parameters();
    bool has_parameters = !parameters.empty();

    std::unique_ptr<Matcher> matcher = has_parameters ? std::unique_ptr<Matcher>(new Matcher(this, parameters.functions())) : nullptr;
    Matcher &sub_matcher = has_parameters ? *matcher : *this;
    for (const auto &g: parameters.functions()) {
        auto clone = sub_matcher.clone(g);
        sub_matcher.assert_matches(g, clone);
        cloned_parameters.add(std::move(clone));
    }

    FunctionRef clone;
    if (f->is_base()) {
        // Create a new function
        clone = Function::make(sub_matcher.convert(f.type()), std::move(cloned_parameters));
    } else {
        // Convert arguments
        std::vector<FunctionRef> converted_arguments;
        for (const auto &argument: f->arguments())
            converted_arguments.push_back(sub_matcher.convert(argument));
        clone = Function::make(sub_matcher.convert(f.type()), std::move(cloned_parameters),
                               sub_matcher.convert(f.base()), std::move(converted_arguments));
    }
    clone->set_name(f->name());
    return clone;
}

FunctionRef Matcher::cheap_clone(const FunctionRef &f) {
    // Should return x itself when x does not depend on any indeterminate (and neither on an indeterminate of some parent)
    for (Matcher *m = this; m != nullptr; m = m->m_parent) {
        if (f->signature_depends_on(m->m_indeterminates))
            return m->clone(f);
    }
    return f;
}

bool Matcher::solved() {
    // A Matcher is `solved` if all indeterminates have a solution
    for (const auto &f: m_indeterminates) {
        if (m_solutions.find(f) == m_solutions.end())
            return false;
    }
    return true;
}

//std::string Matcher::to_string() {
//    std::stringstream ss;
//    ss << "{[";
//    bool first = true;
//    for (auto &f: m_indeterminates) {
//        if (!first)
//            ss << ", ";
//        first = false;
//        ss << Formatter::to_string(f);
//    }
//    ss << "] ";
//
//    first = true;
//    for (auto &m_solution: m_solutions) {
//        if (!first)
//            ss << ", ";
//        first = false;
//        ss << Formatter::to_string(m_solution.first) << " -> " << Formatter::to_string(m_solution.second);
//    }
//    ss << '}';
//    return ss.str();
//}
