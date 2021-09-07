//
// Created by Jesse Vogel on 01/07/2021.
//

#include "specialization.h"

#include <utility>
#include <sstream>

Specialization::Specialization(FunctionPtr base, std::vector<FunctionPtr> arguments, FunctionPtr type,
                               Function::Dependencies dependencies) : Function(std::move(type), std::move(dependencies)),
                                                              m_base(std::move(base)),
                                                              m_arguments(std::move(arguments)) {
    // TODO: maybe some assertions here
}

FunctionPtr Specialization::get_base() {
    return m_base;
}

const std::vector<FunctionPtr> &Specialization::get_arguments() {
    return m_arguments;
}

bool Specialization::depends_on(const std::vector<FunctionPtr> &list) {
    for (auto &arg : m_arguments) {
        if (arg->depends_on(list))
            return true;
    }
    return false;
}

bool Specialization::depends_on(const std::unordered_set<FunctionPtr> &set) {
    for (auto &arg : m_arguments) {
        if (arg->depends_on(set))
            return true;
    }
    return false;
}

std::string Specialization::to_string(bool full, bool with_namespaces) {
    std::ostringstream ss;

    size_t n = m_dependencies.m_functions.size();
    if (n > 0) {
        if (with_namespaces && !m_label.empty()) {
            std::string path = m_space->to_string();
            if (!path.empty())
                ss << path << '.';
        }

        ss << (m_label.empty() ? "λ" : m_label);

        for (int i = 0; i < n; ++i) {
            bool e = m_dependencies.m_explicits[i];
            ss << (e ? " (" : " {");
            ss << m_dependencies.m_functions[i]->to_string(true, with_namespaces);
            ss << (e ? ')' : '}');
        }

        ss << " := ";
    }

    ss << get_base()->to_string(false, with_namespaces);

    for (auto &f : m_arguments) {
        std::string str_argument = f->to_string(false, with_namespaces);
        bool should_enclose = (str_argument.find(' ') != std::string::npos);
        ss << ' ';
        if (should_enclose) ss << '(';
        ss << str_argument;
        if (should_enclose) ss << ')';
    }

    if (full)
        ss << " : " << m_type->to_string(false, with_namespaces);

    return ss.str();
}

