//
// Created by Jesse Vogel on 01/07/2021.
//

#include "Matcher.h"
#include "macros.h"
#include "../parser/Formatter.h"

#include <utility>
#include <algorithm>
#include <memory>

Matcher &Matcher::dummy() {
    static Matcher dummy({});
    return dummy;
}

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

bool Matcher::has_solution(const FunctionRef &f) {
    if (f->is_base()) {
        // For base functions, if f is an indeterminate for any (parent) matcher, f 'has a solution' if it has a match
        for (Matcher *matcher = this; matcher != nullptr; matcher = matcher->m_parent) {
            const auto &indeterminates = matcher->m_indeterminates;
            if (std::find(indeterminates.begin(), indeterminates.end(), f) != indeterminates.end())
                return matcher->m_solutions.find(f) != matcher->m_solutions.end();
        }
        // Otherwise, f is 'its own solution'
        return true;
    } else {
        // For specializations, if both the base and its arguments have solutions, so has f
        return has_solution(f.base()) && std::all_of(f->arguments().begin(), f->arguments().end(), [this](const FunctionRef &g) {
            return has_solution(g);
        });
    }
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
        // Functions must match
        if (!use_matcher.matches(f_parameters.functions()[i], g_parameters.functions()[i]))
            return false;
    }

    // Types should match (note: up to the matches already made by use_matcher)
    if (!use_matcher.matches(f.type(), g.type()))
        return false;

    // At this point, f and g agree up to their signature, i.e. their parameters and types match

    // If f (resp. g) is an indeterminate base function, map f -> g (resp. g -> f).
    // Also treat the case where f or g is an indeterminate of some parent
    if (f->is_base() || g->is_base()) {
        for (Matcher *m = this; m != nullptr; m = m->m_parent) {
            auto it_f = f->is_base() ? std::find(m->m_indeterminates.begin(), m->m_indeterminates.end(), f) : m->m_indeterminates.end();
            auto it_g = g->is_base() ? std::find(m->m_indeterminates.begin(), m->m_indeterminates.end(), g) : m->m_indeterminates.end();
            if (it_f != m->m_indeterminates.end() || it_g != m->m_indeterminates.end()) {
                // If both f and g are indeterminates, we preferably map 'from left to right' in the list of indeterminates, to have some consistency at least
                return (it_f < it_g) ? m->put_solution(f, g) : m->put_solution(g, f);
            }
        }
    }

    // If the bases match, simply match the arguments
    // Note that the bases themselves can be indeterminates, so we have to account for that first.
    const auto &f_base = f.base();
    const auto &g_base = g.base();
    if (f_base != g_base) {
        // Only way for different bases to match is if one of them is an indeterminate
        if (!is_indeterminate(f_base) && !is_indeterminate(g_base))
            return false;
        if (!matches(f_base, g_base))
            return false;
    }

    const auto &f_arguments = f->arguments(), &g_arguments = g->arguments();
    const size_t m = f_arguments.size(); // since the bases already match, we know the number of arguments must agree as well
    for (int i = 0; i < m; ++i) {
        if (!matches(f_arguments[i], g_arguments[i]))
            return false;
    }

    return true;
}

void Matcher::assert_matches(const FunctionRef &f, const FunctionRef &g) {
    bool match = matches(f, g);

    if (!match) {
        Formatter formatter;
        CANARD_LOG(formatter.to_string(*this));
        CANARD_ASSERT(match, "Matching " << formatter.to_string_full(f) << " to " << formatter.to_string_full(g));
    }
}

FunctionRef Matcher::convert(const FunctionRef &f) {
    // If f -> g, return g
    const FunctionRef &g = get_solution(f);
    if (g != nullptr) return g;

    // If f is a base function (and has no solution), then the conversion can only be f itself
    if (f->is_base()) return f; // TODO: if f is an unsolved indeterminate, what should we return ??

    // Convert parameters
    std::unique_ptr<Matcher> matcher;
    Telescope converted_parameters = clone({}, f->parameters(), &matcher);
    Matcher &sub_matcher = converted_parameters.empty() ? *this : *matcher;

    // Otherwise, when f is a specialization, we convert each argument of f
    bool changes = false;
    std::vector<FunctionRef> converted_arguments;
    for (auto &argument: f->arguments()) {
        auto converted_argument = sub_matcher.convert(argument);
        if (!changes && !converted_argument.equivalent(argument))
            changes = true;
        converted_arguments.push_back(std::move(converted_argument));
    }

    // Note: possibly the base should be converted!
    const auto &f_base = f.base();
    const auto &f_base_solution = get_solution(f_base);
    bool f_base_changed = (f_base_solution != nullptr && f_base_solution != f_base);
    const auto &converted_f_base = f_base_changed ? f_base_solution : f_base;
    changes |= f_base_changed;

    // If no actual changes were made to the arguments or the base, we can simply return the original f
    if (!changes) return f;

    // Now we must convert the type of f, this requires another sub_matcher!
    // Because the type is determined by the arguments provided to the base Function,
    // since we replace those now, the type should be re-determined
    auto &f_base_parameters = converted_f_base->parameters();
    Matcher sub_sub_matcher(&sub_matcher, f_base_parameters.functions());
    const size_t m = f_base_parameters.size();
    for (int i = 0; i < m; ++i)
        sub_sub_matcher.assert_matches(f_base_parameters.functions()[i], converted_arguments[i]);

    return Function::make(
            std::move(converted_parameters), sub_sub_matcher.convert(f.type()),
            converted_f_base, std::move(converted_arguments)
    );
}

std::vector<FunctionRef> Matcher::convert(const std::vector<FunctionRef> &fs) {
    std::vector<FunctionRef> converted;
    converted.reserve(fs.size());
    for (const auto &f: fs)
        converted.push_back(convert(f));
    return std::move(converted);
}

Telescope Matcher::clone(const Telescope &parameters, const Telescope &telescope, std::unique_ptr<Matcher> *matcher) {
    // Shortcut
    if (telescope.empty()) return {};

    // Clone the telescope
    Telescope cloned;
    auto sub_matcher = std::unique_ptr<Matcher>(new Matcher(this, telescope.functions()));
    for (const auto &f: telescope.functions()) {
        auto clone = sub_matcher->clone(parameters, f);
        sub_matcher->assert_matches(f, clone.specialize({}, parameters.functions()));
        cloned.add(std::move(clone));
    }

    // Give sub_matcher to whoever is calling
    if (matcher)
        *matcher = std::move(sub_matcher);

    return cloned;
}

FunctionRef Matcher::clone(const FunctionRef &f) {
    return clone({}, f);
}

FunctionRef Matcher::clone(const Telescope &parameters, const FunctionRef &f) {
    // Clone the parameters
    std::unique_ptr<Matcher> matcher;
    Telescope cloned_parameters = clone({}, f->parameters(), &matcher);
    Matcher &sub_matcher = cloned_parameters.empty() ? *this : *matcher;
    // Make a copy of f
    auto clone = f->is_base()
                 ? Function::make(parameters + cloned_parameters, sub_matcher.convert(f.type()))
                 : Function::make(parameters + cloned_parameters, sub_matcher.convert(f.type()),
                                  sub_matcher.convert(f.base()), sub_matcher.convert(f->arguments()));
    clone->set_name(f->name());
    clone->set_implicit(f->implicit());
//    clone->set_constructor(...); // TODO
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
