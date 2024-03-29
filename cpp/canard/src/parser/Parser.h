//
// Created by Jesse Vogel on 01/07/2021.
//

#pragma once

#include "Lexer.h"
#include "Formatter.h"
#include "../data/Session.h"
#include "../data/Context.h"
#include "../searcher/Searcher.h"
#include <unordered_set>

class Parser {
public:

    struct Options {
        bool json = false;
        bool show_namespaces = false;
        bool documentation = false;
        int max_search_depth = 5;
        int max_search_threads = 1;
    };

    Parser(std::istream &, std::ostream &, Session &, Options options);
    Parser(const Parser &) = delete;

    void set_documentation(std::unordered_map<std::string, std::string> *);
    void set_location(std::string &, std::string &);
    bool parse();

private:
    // IO fields
    std::ostream &m_ostream;
    Scanner m_scanner;
    Lexer m_lexer;
    std::string m_filename, m_directory;
    Token m_current_token = {NONE, std::string(), 0, 0};
    bool m_running = false;

    // Namespace related fields
    Context *m_current_namespace;
    std::unordered_set<Context *> m_open_namespaces;
    std::unique_ptr<std::unordered_set<std::string>> m_imported_files;

    // Other fields
    Session &m_session;
    std::unordered_map<std::string, std::string> *m_documentation = nullptr;
    Token m_documentation_token = {NONE, std::string(), 0, 0};
    Options m_options;

    // Searcher
    std::unique_ptr<Searcher> m_searcher;

    // Token methods
    void next_token();
    bool found(TokenType);
    bool found(TokenType, const std::string &);
    Token consume();
    Token consume(TokenType);
    Token consume(TokenType, const std::string &);

    // Parse methods
    bool parse_statement();
    void parse_open();
    void parse_close();
    void parse_import();
    void parse_definition();
    void parse_namespace();
    void parse_structure();
    void parse_check();
    void parse_search();
    void parse_prove();
//    void parse_inspect();
    void parse_docs();
    void parse_debug_search();

    std::string parse_path();
    std::vector<std::string> parse_list_identifiers();
    std::vector<std::string> parse_list_paths();
    std::vector<FunctionRef> parse_functions(Context &, const Telescope &);
    Telescope parse_parameters(Context &);
    Telescope parse_fields(Context &context, const Telescope &);
    FunctionRef parse_expression(Context &, const Telescope &, Context * = nullptr, const std::string * = nullptr);
    FunctionRef parse_term(Context &);
    Context *parse_absolute_namespace();
    std::unordered_set<Context *> parse_namespace_collection();
    int parse_preference();

    // Util
    void setup_searcher();

    // Output methods
    void output(const std::string &);
    void output_search_results(const Telescope &, const std::vector<std::vector<FunctionRef>> &);
    void error(const std::string &);
    std::string format_specialization_exception(SpecializationException &) const;
};

struct ParserException : public std::exception {

    const Token m_token;
    const std::string m_message;

    explicit ParserException(Token token, std::string message) : m_token(std::move(token)),
                                                                 m_message(std::move(message)) {};
};
