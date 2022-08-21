//
// Created by Jesse Vogel on 25/10/2021.
//

#pragma once

#include "../core/Function.h"
#include "../searcher/Query.h"
#include <sstream>

class Formatter {
public:

    inline void show_namespaces(bool b) { m_flag_namespaces = b; }

    std::string format_identifier(const FunctionRef &);
    std::string format_expression(const FunctionRef &);
    std::string format_definition(const FunctionRef &);
    std::string format_telescope(const Telescope &);
    std::string format_matcher(const Matcher &);
    std::string format_query(const Query &);
    std::string format_query_tree(const Query &);

private:

    static const char *INDENT;

    bool m_flag_namespaces = false;

    std::ostringstream ss;

    void write_identifier(const FunctionRef &);
    void write_expression(const FunctionRef &);
    void write_definition(const FunctionRef &);
    void write_telescope(const Telescope &);
    void write_matcher(const Matcher &);
    void write_query(const Query &);

    void clear();

};
