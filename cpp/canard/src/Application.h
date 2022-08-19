//
// Created by Jesse Vogel on 30/09/2021.
//

#pragma once

#include "data/Session.h"
#include "parser/Parser.h"

class Application {
public:

    explicit Application(const std::vector<std::string> &);

    void run();

private:

    Session m_session;
    Parser::Options m_options = {};
    std::unordered_map<std::string, std::string> m_documentation;

    bool parse_file(const std::string &);
    void write_documentation(const std::string &);
    void write_definitions(const std::string &);
};
