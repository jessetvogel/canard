//
// Created by Jesse Vogel on 25/10/2021.
//

#include "Formatter.h"
#include "Namespace.h"
#include <sstream>

std::string Formatter::to_string(const FunctionPtr &f) {
    return to_string(f, false, false);
}

std::string Formatter::to_string(const FunctionPtr &f, bool flag_full, bool flag_namespaces) {
    std::ostringstream ss;
    const bool is_base = (f.base() == f);
    const auto &dependencies = f->parameters();
    const size_t n = dependencies.size();

    // Formatter for base functions
    if (is_base) {
        if (flag_namespaces && f->space() != nullptr) {
            std::string path = f->space()->to_string();
            if (!path.empty()) ss << path << '.';
        }

        ss << (f->label().empty() ? "?" : f->label()); // ss << "[" << f->m_id << "]";

        if (flag_full) {
            for (int i = 0; i < n; ++i) {
                bool e = dependencies.m_explicits[i];
                ss << (e ? " (" : " {");
                ss << to_string(dependencies.m_functions[i], true, flag_namespaces);
                ss << (e ? ')' : '}');
            }
            ss << " : ";
            ss << to_string(f.type(), false, flag_namespaces);
        }
    }
    // Formatter for specializations
    else {
        //    if(full && !m_label.empty()) return m_label;
        if (n > 0) {
            if (flag_namespaces && !f->label().empty() && f->space() != nullptr) {
                std::string path = f->space()->to_string();
                if (!path.empty()) ss << path << '.';
            }

            ss << (f->label().empty() ? "λ" : f->label());

            for (int i = 0; i < n; ++i) {
                bool e = dependencies.m_explicits[i];
                ss << (e ? " (" : " {");
                ss << to_string(dependencies.m_functions[i], true, flag_namespaces);
                ss << (e ? ')' : '}');
            }

            ss << " := ";
        }

        ss << to_string(f.base(), false, flag_namespaces);

        for (auto &g: f->arguments()) {
            std::string str_argument = to_string(g, false, flag_namespaces);
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
