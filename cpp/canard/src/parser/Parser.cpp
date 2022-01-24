//
// Created by Jesse Vogel on 01/07/2021.
//

#include "Parser.h"
#include "Message.h"
#include "../searcher/Searcher.h"
#include "../core/macros.h"
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <fstream>

Parser::Parser(std::istream &istream, std::ostream &ostream, Session &session, Options options)
        : m_ostream(ostream), m_scanner(istream),
          m_lexer(m_scanner),
          m_session(session),
          m_options(options) {
    m_current_namespace = &session.get_global_namespace();
    m_imported_files = std::unique_ptr<std::unordered_set<std::string>>(new std::unordered_set<std::string>());
}

void Parser::next_token() {
    // We want to remember comments for documentation
    if (m_current_token.m_type == COMMENT)
        m_comment_token = std::move(m_current_token);
    else if (m_current_token.m_type != NEWLINE) // remember comments after newlines
        m_comment_token.m_type = NONE;

    m_current_token = m_lexer.get_token();

    if (m_current_token.m_type == NEWLINE) // skip newlines
        next_token();

    if (m_current_token.m_type == COMMENT) // skip comments
        next_token();
}

bool Parser::found(TokenType type) {
    return found(type, std::string());
}

bool Parser::found(TokenType type, const std::string &data) {
    if (m_current_token.m_type == NONE)
        next_token();

    if (type == NONE)
        return true;

    if (m_current_token.m_type == type && (data.empty() || data == m_current_token.m_data))
        return true;

    // If we are not looking for comments, skip the comment and try next token
    if (m_current_token.m_type == TokenType::COMMENT) {
        next_token();
        return found(type, data);
    }

    return false;
}

Token Parser::consume() {
    return consume(NONE, std::string());
}

Token Parser::consume(TokenType type) {
    return consume(type, std::string());
}

Token Parser::consume(TokenType type, const std::string &data) {
    if (found(type, data)) {
        Token token = {NONE};
        std::swap(token, m_current_token);
        return token;
    } else {
        std::ostringstream ss;
        ss << "expected " << data << " (" << type << ")" << " but found " << m_current_token.m_data << " ("
           << m_current_token.m_type << ")";
        throw ParserException(m_current_token, ss.str());
    }
}

bool Parser::parse() {
    try {
        m_running = true;
        while (m_running && parse_statement());
        if (m_running) {
            if (!found(END_OF_FILE))
                throw ParserException(m_current_token, "expected statement, got '" + m_current_token.m_data + "' (" +
                                                       std::to_string(m_current_token.m_type) + ")");
            consume(END_OF_FILE);
            m_running = false;
        }
        return true;
    } catch (ParserException &e) {
        if (m_filename.empty())
            error("Parsing error: " + e.m_message);
        else
            error("Parsing error in " + m_filename + " at " + std::to_string(e.m_token.m_line) + ":" +
                  std::to_string(e.m_token.m_position) + ": " + e.m_message);
        return false;
    } catch (LexerException &e) {
        if (m_filename.empty())
            error("Lexing error: " + e.m_message);
        else
            error("Lexing error in " + m_filename + " at " + std::to_string(e.m_line) + ":" +
                  std::to_string(e.m_position) + ": " + e.m_message);
        return false;
    } catch (std::exception &e) {
        error("Exception: " + std::string(e.what()));
        return false;
    }
}

bool Parser::parse_statement() {
    /* STATEMENT =
            ; | def | search | check | exit
     */

    if (found(SEPARATOR, ";")) {
        consume();
        return true;
    }

    if (found(KEYWORD, "let")) {
        parse_declaration();
        return true;
    }

    if (found(KEYWORD, "def")) {
        parse_definition();
        return true;
    }

    if (found(KEYWORD, "check")) {
        parse_check();
        return true;
    }

    if (found(KEYWORD, "search")) {
        parse_search();
        return true;
    }

    if (found(KEYWORD, "import")) {
        parse_import();
        return true;
    }

    if (found(KEYWORD, "namespace")) {
        parse_namespace();
        return true;
    }

    if (found(KEYWORD, "open")) {
        parse_open();
        return true;
    }

    if (found(KEYWORD, "close")) {
        parse_close();
        return true;
    }

    if (found(KEYWORD, "exit")) {
        consume();
        m_running = false;
        return true;
    }

    if (found(KEYWORD, "inspect")) {
        parse_inspect();
        return true;
    }

    if (found(KEYWORD, "doc")) {
        parse_doc();
        return true;
    }

    return false;
}

void Parser::parse_inspect() {
    /*
        inspect NAMESPACE
    */

    Token t_inspect = consume(KEYWORD, "inspect");

    // Get namespace
    std::string path = parse_path();
    auto space = m_session.get_global_namespace().get_namespace(path);
    if (space == nullptr)
        throw ParserException(t_inspect, "unknown namespace '" + path + "'");

    // Construct list of labels
    std::vector<std::string> labels;
    for (auto &f: space->get_functions()) {
        std::string label = f->label();
        if (!label.empty())
            labels.push_back(label);
    }

    // Output
    if (m_options.json)
        output(Message::create(SUCCESS, labels));
    else
        for (auto &label: labels)
            output(label);
}

void Parser::parse_doc() {
    /*
        inspect IDENTIFIER
    */

    Token t_doc = consume(KEYWORD, "doc");
    auto f = parse_expression(m_current_namespace->get_context());

    std::string doc;
    if (m_options.documentation && m_documentation != nullptr) {
        auto it = m_documentation->find(f);
        if (it != m_documentation->end())
            doc = it->second;
    }

    if (m_options.json)
        output(Message::create(SUCCESS, doc));
    else
        output("📝 " + (doc.empty() ? "No documentation" : doc));
}

void Parser::parse_open() {
    /*
        open NAMESPACE
     */

    Token t_open = consume(KEYWORD, "open");
    std::string path = parse_path();
    auto space = m_session.get_global_namespace().get_namespace(path);
    if (space == nullptr)
        throw ParserException(t_open, "invalid namespace name '" + path + "'");
    m_open_namespaces.insert(space);
}

void Parser::parse_close() {
    /*
        close NAMESPACE
     */

    Token t_close = consume(KEYWORD, "close");
    std::string path = parse_path();
    auto space = m_session.get_global_namespace().get_namespace(path);
    if (space == nullptr)
        throw ParserException(t_close, "invalid namespace name '" + path + "'");
    m_open_namespaces.erase(space);
}

void Parser::parse_import() {
    /*
        import "PATH"
     */

    Token t_import = consume(KEYWORD, "import");
    std::string filename = consume(STRING).m_data;
    filename = filename.substr(1, filename.length() - 2);

    // Convert path to normalized path
    std::string path = m_directory + filename;
    char normalized_path[PATH_MAX]; // PATH_MAX includes the \0 so +1 is not required
    char *result = realpath(path.c_str(), normalized_path);
    if (!result) {
        if (errno == ENOENT)
            throw ParserException(t_import, "could not find '" + filename + "'");
        else
            throw ParserException(t_import, "could not open '" + filename + "'");
    }
    path = normalized_path;

    // If file was already opened once, skip it
    if (m_imported_files->find(path) != m_imported_files->end())
        return;

    // Add the absolute path to list of imported files
    m_imported_files->insert(path);

    // Create input file stream
    std::ifstream ifstream(normalized_path);
    if (!ifstream.good())
        throw ParserException(t_import, "could not open '" + filename + "'");

    // Obtain directory and filename
    size_t i = path.find_last_of('/');
    std::string directory = path.substr(0, i + 1);
    std::string file = path.substr(i + 1);

    // Create sub_parser to parse the file
    Parser sub_parser(ifstream, m_ostream, m_session, m_options);
    sub_parser.set_location(directory, file);
    sub_parser.set_documentation(m_documentation);
    sub_parser.m_imported_files = std::move(m_imported_files);
    bool success = sub_parser.parse();
    m_imported_files = std::move(sub_parser.m_imported_files);

    if (!success)
        throw ParserException(t_import, "error in importing '" + filename + "'");
}

void Parser::parse_namespace() {
    /*
        namespace IDENTIFIER
            STATEMENT*
        end
     */

    consume(KEYWORD, "namespace");
    std::string name = consume(IDENTIFIER).m_data;
    // If namespace already exists, use that one. Otherwise, create a new one
    auto space = m_current_namespace->get_namespace(name);
    m_current_namespace = (space != nullptr) ? space : m_current_namespace->create_subspace(name);

    while (parse_statement());

    consume(KEYWORD, "end");
    consume(IDENTIFIER, name);
    m_current_namespace = m_current_namespace->get_parent();
}

void Parser::parse_search() {
    /*
        search LIST_OF_PARAMETERS
     */

    consume(KEYWORD, "search");

    // Parse indeterminates
    std::vector<FunctionPtr> indeterminates;
    Context sub_context(m_current_namespace->get_context());
    while (found(SEPARATOR, "(")) {
        consume();
        auto functions = parse_functions(sub_context);
        indeterminates.insert(indeterminates.end(), functions.begin(), functions.end());
        consume(SEPARATOR, ")");
    }

    // Create searcher and specify the searching space
    Searcher searcher(m_options.max_search_depth, m_options.max_search_threads);
    std::unordered_set<Namespace *> added_namespaces;
    for (auto space = m_current_namespace; space != nullptr; space = space->get_parent()) {
        searcher.add_namespace(*space);
        added_namespaces.insert(space);
    }
    for (auto &space: m_open_namespaces) {
        if (added_namespaces.find(space) == added_namespaces.end())
            searcher.add_namespace(*space);
    }

    // Make a query and do a search
    // Store the results in a list
    auto query = std::make_shared<Query>(indeterminates);

    auto start_time = std::chrono::system_clock::now();
    bool success = searcher.search(query);
    auto end_time = std::chrono::system_clock::now();

    auto result = searcher.result();

    if (m_options.json) {
        if (success) {
            std::vector<std::string> keys, values;
            keys.reserve(indeterminates.size());
            values.reserve(result.size());
            for (auto &f: indeterminates)
                keys.push_back(f->label());
            for (auto &g: result)
                values.push_back(format(g));
            output(Message::create(SUCCESS, keys, values));
        } else {
            output(Message::create(SUCCESS, std::vector<std::string>()));
        }
    } else {
        if (!success) {
            output("🥺 no solutions found");
            return;
        }

        std::stringstream ss;
        ss << "🔎 ";
        size_t n = result.size();
        bool first = true;
        for (int i = 0; i < n; ++i) {
            if (!first) ss << ", ";
            first = false;
            ss << indeterminates[i]->label() << " = " << format(result[i]);
        }
        output(ss.str());
    }

    CANARD_LOG("Search took " << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count()
                              << " ms");
}

void Parser::parse_check() {
    /*
        check EXPRESSION
     */

    consume(KEYWORD, "check");
    auto f_str = Formatter::to_string(parse_expression(m_current_namespace->get_context()), true, m_options.explict);

    if (m_options.json)
        output(Message::create(SUCCESS, f_str));
    else
        output("🦆 " + f_str);
}

void Parser::parse_declaration() {
    /*
        let IDENTIFIER DEPENDENCIES : TYPE
     */

    consume(KEYWORD, "let");

    bool has_doc = (m_documentation != nullptr && m_comment_token.m_type == COMMENT);
    std::string doc = has_doc ? m_comment_token.m_data : "";

    for (auto &f: parse_functions(m_current_namespace->get_context())) {
        m_current_namespace->put_function(f);

        // Store documentation
        if (has_doc) m_documentation->emplace(f, doc);
    }
}

void Parser::parse_definition() {
    /*
        def IDENTIFIER DEPENDENCIES := EXPRESSION
     */

    Token t_def = consume(KEYWORD, "def");

    bool has_doc = (m_documentation != nullptr && m_comment_token.m_type == COMMENT);
    std::string doc = has_doc ? m_comment_token.m_data : "";

    std::string identifier = consume(IDENTIFIER).m_data;

    // Parse dependencies
    Context &context = m_current_namespace->get_context();
    Context sub_context(m_current_namespace->get_context());
    FunctionParameters dependencies = parse_parameters(sub_context);
    // (if there are dependencies, we will use the sub_context for any further use)
    Context &use_context = (dependencies.size() > 0) ? sub_context : context;

    consume(SEPARATOR, ":=");

    auto f = parse_expression(use_context, std::move(dependencies));
    if (!context.put_function(identifier, f))
        throw ParserException(t_def, "name " + identifier + " already used in this context");

    m_current_namespace->put_function(f);

    // Store documentation
    if (has_doc)
        m_documentation->emplace(f, doc);
}

std::string Parser::parse_path() {
    /*
        PATH = IDENTIFIER ( . IDENTIFIER )*
     */

    std::ostringstream ss;
    ss << consume(IDENTIFIER).m_data;
    while (found(SEPARATOR, ".")) {
        ss << consume().m_data;
        ss << consume(IDENTIFIER).m_data;
    }
    return ss.str();
}

std::vector<std::string> Parser::parse_list_identifiers() {
    /*
        LIST_OF_IDENTIFIERS = IDENTIFIER+
     */

    std::vector<std::string> list;
    do list.push_back(consume(IDENTIFIER).m_data);
    while (found(IDENTIFIER));
    return list;
}

std::vector<FunctionPtr> Parser::parse_functions(Context &context) {
    /* FUNCTIONS =
        LIST_OF_IDENTIFIERS PARAMETERS : OUTPUT_TYPE
     */

    auto identifiers = parse_list_identifiers();

    // Parse parameters
    Context sub_context(context);
    FunctionParameters parameters = parse_parameters(sub_context);
    Context &use_context = (parameters.size() > 0) ? sub_context : context;

    // Parse type
    consume(SEPARATOR, ":");
    Token token = m_current_token;
    auto type = parse_expression(use_context);
    if (type.type() != m_session.TYPE && type.type() != m_session.PROP)
        throw ParserException(token, "expected a Type or Prop");
    if (type->parameters().size() > 0) // TODO: I don't think this is expected behaviour..
        throw ParserException(token, "forgot arguments");

    // Actually create the functions
    std::vector<FunctionPtr> output;
    for (std::string &identifier: identifiers) {
        // don't move, since we use the parameters multiple times. Could make it shared in the future?
        auto f = type.pool().create_function(type, parameters);
        if (!context.put_function(identifier, f))
            throw ParserException(m_current_token, "identifier '" + identifier + "' already used in this context");
        output.push_back(f);
    }

    return output;
}

FunctionParameters Parser::parse_parameters(Context &context) {
    FunctionParameters dependencies;
    bool is_explicit;
    while ((is_explicit = found(SEPARATOR, "(")) || found(SEPARATOR, "{")) {
        consume();
        for (auto &f: parse_functions(context)) {
            dependencies.m_functions.push_back(f);
            dependencies.m_explicits.push_back(is_explicit);
        }
        consume(SEPARATOR, is_explicit ? ")" : "}");
    }

    // Check if the implicit dependencies were actually used
    size_t n = dependencies.size();
    for (int i = 0; i < n; ++i) {
        if (!dependencies.m_explicits[i] && !context.is_used(dependencies.m_functions[i]))
            throw ParserException(m_current_token,
                                  "implicit parameter '" + Formatter::to_string(dependencies.m_functions[i]) +
                                  "' unused");
    }

    return dependencies;
}

FunctionPtr Parser::parse_expression(Context &context) {
    return parse_expression(context, {});
}

FunctionPtr Parser::parse_expression(Context &context, FunctionParameters dependencies) {
    /* EXPRESSION =
        TERM | TERM TERM+
     */

    // Get a list of following terms
    std::vector<FunctionPtr> terms;
    for (auto term = parse_term(context); term != nullptr; term = parse_term(context))
        terms.push_back(term);

    // If there are no terms
    size_t n = terms.size();
    if (n == 0)
        throw ParserException(m_current_token, "expected expression but not found");

    FunctionPtr base = terms[0];

    // If there is only one term and no dependencies, return that term
    // If there are dependencies, specialize the term with its own arguments, but with dependencies!
    if (n == 1) {
        if (dependencies.size() == 0)
            return base;
        try {
            return base.specialize(base->arguments(), std::move(dependencies));
        } catch (SpecializationException &e) {
            throw ParserException(m_current_token, e.m_message);
        }
    }

    // If there is more than one term, specialize
    terms.erase(terms.begin());
    try {
        return base.specialize(terms, std::move(dependencies));
    } catch (SpecializationException &e) {
        throw ParserException(m_current_token, e.m_message);
    }
}

FunctionPtr Parser::parse_term(Context &context) {
    /* TERM =
        PATH | ( EXPRESSION )
     */

    if (found(IDENTIFIER)) {
        Token token = m_current_token;
        std::string path = parse_path();
        // First look through the context
        auto f = context.get_function(path);
        if (f != nullptr) return f;

        // Then look through namespaces from the current one to the top
        for (auto space = m_current_namespace; space != nullptr; space = space->get_parent()) {
            f = space->get_function(path);
            if (f != nullptr) return f;
        }
        // If that failed, look through the other open namespaces
        std::vector<FunctionPtr> candidates;
        for (auto &space: m_open_namespaces) {
            f = space->get_function(path);
            if (f != nullptr) candidates.push_back(f);
        }
        // If there is a unique candidate, we are good
        if (candidates.size() == 1)
            return candidates[0];
        // If more than one candidate, its ambiguous
        if (candidates.size() > 1) {
            std::ostringstream ss;
            for (int i = 0; i < candidates.size(); ++i) {
                if (i == candidates.size() - 1) ss << " or ";
                ss << Formatter::to_string(candidates[i], false, true);
                if (i < candidates.size() - 2) ss << ", ";
            }
            throw ParserException(token, "ambiguous identifier '" + path + "', could be " + ss.str());
        }
        // Otherwise, if no candidates, throw unknown
        throw ParserException(token, "unknown identifier '" + path + "'");
    }

    if (found(SEPARATOR, "(")) {
        consume();
        auto expression = parse_expression(context);
        consume(SEPARATOR, ")");
        return expression;
    }

    return nullptr;
}

void Parser::output(const std::string &message) {
    m_ostream << message << std::endl;
}

void Parser::error(const std::string &message) {
    if (m_options.json)
        output(R"({"status":"error","data":")" + message + "\"}");
    else
        output("⚠️ " + message);
}

void Parser::set_location(std::string &directory, std::string &filename) {
    m_directory = directory;
    m_filename = filename;
}

void Parser::set_documentation(std::unordered_map<FunctionPtr, std::string> *documentation) {
    m_documentation = documentation;
}

std::string Parser::format(const FunctionPtr &f) const {
    return Formatter::to_string(f, false, m_options.explict);
}
