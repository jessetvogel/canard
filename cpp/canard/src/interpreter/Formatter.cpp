//
// Created by Jesse Vogel on 25/10/2021.
//

#include "Formatter.h"
#include "Namespace.h"
#include "Metadata.h"
#include <sstream>

std::string Formatter::to_string(const FunctionRef &f) {
    return to_string(f, false, false);
}

std::string Formatter::to_string(const FunctionRef &f, bool flag_full, bool flag_namespaces) {
    std::ostringstream ss;
    const bool is_base = (f.base() == f);
    const auto &parameters = f->parameters();
    const size_t n = parameters.size();
    auto metadata = (Metadata *) f->metadata();
    auto space = metadata ? metadata->m_space : nullptr;

    // Formatter for base functions
    if (is_base) {
        if (flag_namespaces && space) {
            std::string path = space->to_string();
            if (!path.empty()) ss << path << '.';
        }

        ss << (f->name().empty() ? "?" : f->name()); // ss << "[" << f->m_id << "]";

        if (flag_full) {
            for (int i = 0; i < n; ++i) {
                bool e = !metadata || metadata->m_explicit_parameters[i];
                ss << (e ? " (" : " {");
                ss << to_string(parameters.functions()[i], true, flag_namespaces);
                ss << (e ? ')' : '}');
            }
            ss << " : ";
            ss << to_string(f.type(), false, flag_namespaces);
        }
    }
        // Formatter for specializations
    else {
//        if(full && !m_label.empty()) return m_label;
        if (n > 0) {
            if (flag_namespaces && !f->name().empty() && space) {
                std::string path = space->to_string();
                if (!path.empty()) ss << path << '.';
            }

            ss << (f->name().empty() ? "λ" : f->name());

            for (int i = 0; i < n; ++i) {
                bool e = !metadata || metadata->m_explicit_parameters[i];
                ss << (e ? " (" : " {");
                ss << to_string(parameters.functions()[i], true, flag_namespaces);
                ss << (e ? ')' : '}');
            }

            ss << " := ";
        }

        ss << to_string(f.base(), false, flag_namespaces);

        auto f_base_metadata = (Metadata *) f.base()->metadata();
        const auto m = f->arguments().size();
        for (int i = 0; i < m; ++i) {
            // Skip implicit arguments
            if (f_base_metadata && !f_base_metadata->m_explicit_parameters[i])
                continue;

            // Stringify the argument, and possibly enclose it with parentheses
            std::string str_argument = to_string(f->arguments()[i], false, flag_namespaces);
            bool should_enclose = (str_argument.find(' ') != std::string::npos);
            ss << ' ';
            if (should_enclose) ss << '(';
            ss << str_argument;
            if (should_enclose) ss << ')';
        }

        if (flag_full)
            ss << " : " << to_string(f.type(), false, flag_namespaces);
    }
    return ss.str();
}
