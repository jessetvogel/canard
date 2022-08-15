//
// Created by Jesse Vogel on 25/10/2021.
//

#pragma once

#include "../core/Function.h"
#include "../searcher/Query.h"

class Formatter {
public:

    std::string to_string(const FunctionRef &);
    std::string to_string_full(const FunctionRef &);
    std::string to_string(const Telescope &);
    std::string to_string(const Query &);
    std::string to_string(const Matcher &);

    inline void show_namespaces(bool b) { m_flag_namespaces = b; }

private:

    static const char *INDENT;

    bool m_flag_namespaces = false;

    std::string format_expression(const FunctionRef &);

};
