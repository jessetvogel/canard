//
// Created by Jesse Vogel on 30/09/2021.
//

#pragma once

#include "core/Session.h"
#include "parser/Parser.h"

class Application {
public:

    explicit Application(const std::vector<std::string> &);

    void run();

private:

    bool m_flag_json = false;
    bool m_flag_explicit = false;
    bool m_flag_documentation = false;
    bool m_flag_english = false;

    Session m_session;
    std::unordered_map<FunctionPtr, std::string> m_documentation;

    bool parse_file(const std::string &);
    void parser_set_flags(Parser&);

//    Parser create_parser(std::istream &, std::ostream &);
};


