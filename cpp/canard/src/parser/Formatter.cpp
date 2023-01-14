//
// Created by Jesse Vogel on 25/10/2021.
//

#include "Formatter.h"
#include <algorithm>

const char *Formatter::INDENT = "  ";

std::string Formatter::format_identifier(const FunctionRef &f) {
    clear();
    write_identifier(f);
    return ss.str();
}

std::string Formatter::format_expression(const FunctionRef &f) {
    clear();
    write_expression(f);
    return ss.str();
}

std::string Formatter::format_definition(const FunctionRef &f) {
    clear();
    write_definition(f);
    return ss.str();
}

std::string Formatter::format_telescope(const Telescope &telescope) {
    clear();
    write_telescope(telescope);
    return ss.str();
}

std::string Formatter::format_matcher(const Matcher &matcher) {
    clear();
    write_matcher(matcher);
    return ss.str();
}

std::string Formatter::format_query(const Query &query) {
    clear();
    write_query(query);
    return ss.str();
}

std::string Formatter::format_query_tree(const Query &query) {
    clear();
    std::vector<const Query *> chain;
    for (const Query *q = &query; q != nullptr; q = q->parent().get())
        chain.push_back(q);
    std::reverse(chain.begin(), chain.end());
    for (const auto q: chain) {
        for (const auto &entry: q->solutions()) {
            ss << "solved for ";
            write_definition(entry.first);
            ss << " with ";
            write_definition(entry.second);
            ss << "\n";
        }
        write_query(*q);
        ss << "\n";
    }
    return ss.str();
}

void Formatter::write_identifier(const FunctionRef &f) {
    if (f->name().empty()) {
        ss << "λ";
        return;
    }
    if (m_flag_namespaces && f->space()) {
        const std::string &path = ((Context *) f->space())->full_name();
        if (!path.empty())
            ss << path << '.';
    }
    ss << f->name();
}

void Formatter::write_expression(const FunctionRef &f) {
    if (f->is_base())
        return write_identifier(f);

    if (!f->parameters().empty()) {
        write_identifier(f);
        write_telescope(f->parameters());
        ss << " := ";
    }

    if (f.base()->is_constructor()) {
        const auto &constructor = f.base();
        write_expression(f.type());
        ss << " {";
        const auto n = constructor->parameters().size();
        bool first = true;
        for (int i = (int) f.type().base()->parameters().size(); i < n; ++i, first = false) { // lol this for loop
            ss << (first ? " " : ", ");
            ss << constructor->parameters().functions()[i]->name() << " := ";
            write_expression(f->arguments()[i]);
        }
        ss << " }";
        return;
    }

    write_identifier(f.base());
    const auto &f_base_parameters = f.base()->parameters().functions();
    const auto m = f_base_parameters.size();
    for (int i = 0; i < (int) m; ++i) {
        // Skip implicit arguments
        if (f_base_parameters[i]->implicit())
            continue;
        // Stringify the argument, and possibly enclose it with parentheses
        const auto &g = f->arguments()[i];
        bool should_enclose = !g->is_base(); // bool should_enclose = (argument.find(' ') != std::string::npos);
        ss << ' ';
        if (should_enclose) ss << '(';
        write_expression(g);
        if (should_enclose) ss << ')';
    }
}

void Formatter::write_definition(const FunctionRef &f) {
    if (f->is_base()) {
        if (f->constructor())
            ss << "structure ";

        write_identifier(f);
        write_telescope(f->parameters());

        // If f is a structure, (i.e. has a constructor), also write the fields
        if (f->constructor()) {
            ss << " := {";
            const auto &fields = f->constructor()->parameters().functions();
            bool first = true;
            for (auto it = fields.begin() + (int) f->parameters().size(); it != fields.end(); ++it, first = false) {
                ss << (first ? " " : ", ");
                write_definition(*it);
            }
            ss << " }";
            return;
        }

        // ... otherwise, just write the type
        ss << " : ";
        write_expression(f.type());
        return;
    }

    return write_expression(f);
}

void Formatter::write_telescope(const Telescope &telescope) {
    const auto &functions = telescope.functions();
    for (auto it = functions.begin(); it != functions.end();) {
        bool implicit = (*it)->implicit();

        // Find maximal sequence of equivalent functions: group them for printing
        std::vector<FunctionRef> singleton = {*it};
        auto it_other = it + 1;
        for (; it_other != functions.end(); ++it_other) {
            if ((*it_other)->implicit() != implicit)
                break;
            Matcher matcher(singleton);
            if (!matcher.matches(*it, *it_other))
                break;
        }

        // Print group of functions in between brackets
        ss << (implicit ? " {" : " (");
        for (auto it_sub = it; it_sub != it_other - 1; ++it_sub)
            ss << (*it_sub)->name() << ' '; // for all but the last we only need the name
        write_definition(*(it_other - 1)); // for the last we want the full description
        ss << (implicit ? '}' : ')');

        // Continue where we left off
        it = it_other;
    }
}

void Formatter::write_matcher(const Matcher &matcher) {
    ss << "{\n";
    for (const auto &f: matcher.indeterminates()) {
        const auto &g = matcher.get_solution(f);
        ss << INDENT;
        write_definition(f);
        ss << " => ";
        if (g == nullptr)
            ss << "?";
        else
            write_definition(g);
        ss << "\n";
    }
    ss << "}";
}

void Formatter::write_query(const Query &query) {
    ss << "{\n";
    const auto &locals = query.locals();
    for (int i = 0; i < locals.size(); ++i) {
        ss << "Locals @ " << i << ": ";
        for (const auto &f: locals[i]) {
            write_definition(f);
            ss << ", ";
        }
        ss << "\n";
    }
    const auto n = query.telescope().size();
    for (int i = 0; i < n; ++i) {
        ss << INDENT;
        write_definition(query.telescope().functions()[i]);
        ss << "; depth = " << query.depths()[i];
        ss << "; context_depth = " << query.locals_depths()[i] << "\n";
    }
    ss << "}";
}

void Formatter::clear() {
//    std::ostringstream().swap(ss);
    ss.clear();
    ss.str(std::string());
}
