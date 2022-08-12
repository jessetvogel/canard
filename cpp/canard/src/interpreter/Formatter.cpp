//
// Created by Jesse Vogel on 25/10/2021.
//

#include "Formatter.h"
#include "Namespace.h"
#include "Metadata.h"
#include <sstream>

const char *Formatter::INDENT = "  ";

std::string Formatter::to_string(const FunctionRef &f) {
    // For base functions and specializations with a name, just use the name (possibly with namespace)
    if (f->is_base() || !f->name().empty()) {
        std::ostringstream ss;
        auto metadata = (Metadata *) f->metadata();
        auto space = metadata ? metadata->m_space : nullptr;
        if (m_flag_namespaces && space) {
            std::string path = space->full_name();
            if (!path.empty()) ss << path << '.';
        }
        ss << (f->name().empty() ? "?" : f->name());
        return ss.str();
    }

    // For specializations with no parameters, print the expression
    if (f->parameters().empty())
        return format_expression(f);

    // For specializations with parameters, fall back to the full string
    return to_string_full(f);
}

std::string Formatter::format_expression(const FunctionRef &f) {
    if (f->is_base())
        return to_string(f);
    else {
        std::ostringstream ss;
        ss << to_string(f.base());
        const auto &f_base_parameters = f.base()->parameters().functions();
        const auto m = f_base_parameters.size();
        for (int i = 0; i < m; ++i) {
            // Skip implicit arguments
            auto parameter_metadata = (Metadata *) f_base_parameters[i]->metadata();
            if (parameter_metadata && parameter_metadata->m_implicit)
                continue;

            // Stringify the argument, and possibly enclose it with parentheses
            std::string str_argument = to_string(f->arguments()[i]);
            bool should_enclose = (str_argument.find(' ') != std::string::npos);
            ss << ' ';
            if (should_enclose) ss << '(';
            ss << str_argument;
            if (should_enclose) ss << ')';
        }
        return ss.str();
    }
}

std::string Formatter::to_string_full(const FunctionRef &f) {
    std::ostringstream ss;

    if (f->is_base()) {
        // Formatter for base functions
        ss << to_string(f);
        ss << to_string(f->parameters());

        // If it is a structure (i.e. has a constructor), also print the fields
        auto metadata = (Metadata *) f->metadata();
        if (metadata && metadata->m_constructor != nullptr) {
            ss << " {\n";
            const auto &fields = metadata->m_constructor->parameters().functions();
            for (auto it = fields.begin() + (int) f->parameters().size(); it != fields.end(); ++it)
                ss << INDENT << to_string_full(*it) << ((it == fields.end() - 1) ? "\n" : ",\n");
            ss << "}";
        } else {
            // Otherwise, just print the type
            ss << " : " << to_string(f.type());
        }
    } else {
        // Formatter for specializations
        ss << (f->name().empty() ? "λ" : to_string(f));
        ss << to_string(f->parameters());
        ss << " := ";
        ss << format_expression(f);
    }

    return ss.str();
}

std::string Formatter::to_string(const Telescope &telescope) {
    std::ostringstream ss;

    const auto &functions = telescope.functions();
    for (auto it = functions.begin(); it != functions.end();) {
        bool implicit = (*it)->metadata() && ((Metadata *) (*it)->metadata())->m_implicit;

        // Find maximal sequence of equivalent functions: group them for printing
        std::vector<FunctionRef> singleton = {*it};
        auto it_other = it + 1;
        for (; it_other != functions.end(); ++it_other) {
            bool implicit_other = (*it_other)->metadata() && ((Metadata *) (*it_other)->metadata())->m_implicit;
            if (implicit_other != implicit)
                break;
            Matcher matcher(singleton);
            if (!matcher.matches(*it, *it_other))
                break;
        }

        // Print group of functions in between brackets
        ss << (implicit ? " {" : " (");
        for (auto it_sub = it; it_sub != it_other - 1; ++it_sub)
            ss << to_string(*it_sub) << ' '; // for all but the last we only need the name
        ss << to_string_full(*(it_other - 1)); // for the last we want the full description
        ss << (implicit ? '}' : ')');

        // Continue where we left off
        it = it_other;
    }

    return ss.str();
}

std::string Formatter::to_string(const Query &query) {
    std::ostringstream ss;
    ss << "Query@" << query.depth() << " {";
    for (auto &f: query.locals())
        ss << " (" << to_string_full(f) << ")";
    ss << " } ";
    for (auto &f: query.indeterminates())
        ss << " (" << to_string_full(f) << ")";
    return ss.str();
}
