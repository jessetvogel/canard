//
// Created by Jesse Vogel on 01/07/2021.
//

#pragma once

#include <ostream>
#include <unordered_set>
#include <utility>
#include "Lexer.h"
#include "../core/Session.h"
#include "../core/Context.h"

class Parser {

    std::ostream &m_ostream;
    Scanner m_scanner;
    Lexer m_lexer;
    std::string m_filename, m_directory;
    Token m_current_token = {NONE};
    bool m_running = false;

    Session &m_session;
    Namespace *m_current_namespace;
    std::unordered_set<Namespace *> m_open_namespaces;
    std::unique_ptr<std::unordered_set<std::string>> m_imported_files;

    bool m_use_json = false;
    bool m_use_explicit = false;

    Token m_comment_token = {NONE};

    std::unordered_map<FunctionPtr, std::string> *m_documentation = nullptr;

    // get_token methods

    void next_token();

    bool found(TokenType);

    bool found(TokenType, const std::string &);

    Token consume();

    Token consume(TokenType);

    Token consume(TokenType, const std::string &);

    // parsing methods
    bool parse_statement();

    void parse_inspect();

    void parse_open();

    void parse_close();

    void parse_import();

    void parse_namespace();

    void parse_search();

    void parse_check();

    void parse_declaration();

    void parse_definition();

    std::string parse_path();

    std::vector<std::string> parse_list_identifiers();

    std::vector<FunctionPtr> parse_functions(Context &);

    Function::Dependencies parse_dependencies(Context &);

    FunctionPtr parse_expression(Context &);

    FunctionPtr parse_expression(Context &, Function::Dependencies);

    FunctionPtr parse_term(Context &);

    // output methods
    void output(const std::string &);

    void error(const std::string &);

public:

    Parser(std::istream &, std::ostream &, Session &);
    Parser(const Parser&) = delete;

    void use_json(bool);

    void use_explicit(bool);

    void set_documentation(std::unordered_map<FunctionPtr, std::string> *);

    bool parse();

    void set_location(std::string &, std::string &);
    void parse_doc();
};

struct ParserException : public std::exception {

    const Token m_token;
    const std::string m_message;

    explicit ParserException(Token token, std::string message) : m_token(std::move(token)),
                                                                 m_message(std::move(message)) {};
};
