//
// Created by Jesse Vogel on 01/07/2021.
//

#pragma once

#include "Lexer.h"
#include "../core/Session.h"
#include "../core/Context.h"
#include "../core/Formatter.h"

class Parser {
public:

    struct Options {
        bool json = false;
        bool explict = false;
        bool documentation = false;
        int max_search_depth = 5;
        uint32_t max_search_threads = 1;
    };

    Parser(std::istream &, std::ostream &, Session &, Options options);
    Parser(const Parser &) = delete;

    void set_documentation(std::unordered_map<FunctionPtr, std::string> *);
    void set_location(std::string &, std::string &);
    bool parse();

private:
    // IO fields
    std::ostream &m_ostream;
    Scanner m_scanner;
    Lexer m_lexer;
    std::string m_filename, m_directory;
    Token m_current_token = {NONE};
    bool m_running = false;

    // Namespace related fields
    Namespace *m_current_namespace;
    std::unordered_set<Namespace *> m_open_namespaces;
    std::unique_ptr<std::unordered_set<std::string>> m_imported_files;

    // Other fields
    Session &m_session;
    std::unordered_map<FunctionPtr, std::string> *m_documentation = nullptr;
    Token m_comment_token = {NONE};
    Options m_options;

    // Token methods
    void next_token();
    bool found(TokenType);
    bool found(TokenType, const std::string &);
    Token consume();
    Token consume(TokenType);
    Token consume(TokenType, const std::string &);

    // Parse methods
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
    void parse_doc();

    std::string parse_path();
    std::vector<std::string> parse_list_identifiers();
    std::vector<FunctionPtr> parse_functions(Context &);
    Function::Dependencies parse_dependencies(Context &);
    FunctionPtr parse_expression(Context &);
    FunctionPtr parse_expression(Context &, Function::Dependencies);
    FunctionPtr parse_term(Context &);

    // Formatting method
    std::string format(const FunctionPtr &f) const;

    // Output methods
    void output(const std::string &);
    void error(const std::string &);
};

struct ParserException : public std::exception {

    const Token m_token;
    const std::string m_message;

    explicit ParserException(Token token, std::string message) : m_token(std::move(token)),
                                                                 m_message(std::move(message)) {};
};
