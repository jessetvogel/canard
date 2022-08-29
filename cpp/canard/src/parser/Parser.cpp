//
// Created by Jesse Vogel on 01/07/2021.
//

#include "Parser.h"
#include "Message.h"
#include "../searcher/Searcher.h"
#include "../core/macros.h"
#include "../searcher/DebugSearcher.h"
#include <fstream>
#include <algorithm>

Parser::Parser(std::istream &istream, std::ostream &ostream, Session &session, Options options)
        : m_ostream(ostream), m_scanner(istream),
          m_lexer(m_scanner),
          m_session(session),
          m_options(options) {
    m_current_namespace = &session.global_namespace();
    m_imported_files = std::unique_ptr<std::unordered_set<std::string>>(new std::unordered_set<std::string>());
}

void Parser::next_token() {
    // We want to remember comments for documentation
    if (m_current_token.m_type == COMMENT)
        m_last_comment_token = std::move(m_current_token);
    else if (m_current_token.m_type != NEWLINE) // remember comments only after newlines
        m_last_comment_token.m_type = NONE;

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
        ss << "expected " << Lexer::to_string(type) << " " << data
           << " but found " << Lexer::to_string(m_current_token.m_type) << " " << m_current_token.m_data;
        throw ParserException(m_current_token, ss.str());
    }
}

std::string to_path(const Namespace &space, const std::string &name) {
    return space.name().empty() ? name : space.full_name() + '.' + name;
}

void ltrim(std::string &s) { // trim from start (in place)
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

void rtrim(std::string &s) { // trim from end (in place)
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

void trim(std::string &s) { // trim from both ends (in place)
    ltrim(s);
    rtrim(s);
}

bool Parser::parse() {
    try {
        m_running = true;
        while (m_running && parse_statement());
        if (m_running) {
            if (!found(END_OF_FILE)) {
                std::ostringstream ss;
                ss << "expected statement, but found " << Lexer::to_string(m_current_token.m_type) << " " << m_current_token.m_data;
                throw ParserException(m_current_token, ss.str());
            }
            consume(END_OF_FILE);
            m_running = false;
        }
        return true;
    } catch (ParserException &e) {
        std::string prefix =
                m_filename.empty()
                ? "Parsing error: "
                : "Parsing error in " + m_filename + " at " + std::to_string(e.m_token.m_line) + ":" + std::to_string(e.m_token.m_position) + ": ";
        error(prefix + e.m_message);
        return false;
    } catch (LexerException &e) {
        std::string prefix =
                m_filename.empty()
                ? "Lexing error: "
                : "Lexing error in " + m_filename + " at " + std::to_string(e.m_line) + ":" + std::to_string(e.m_position) + ": ";
        error(prefix + e.m_message);
        return false;
    }
    catch (std::exception &e) {
        error("Exception: " + std::string(e.what()));
        return false;
    }
}

bool Parser::parse_statement() {
    /*
        STATEMENT = ; | let | check | search | import | namespace | structure | open | close | exit | inspect | doc
     */

    if (found(SEPARATOR, ";")) {
        consume();
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

    if (found(KEYWORD, "prove")) {
        parse_prove();
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

    if (found(KEYWORD, "docs")) {
        parse_docs();
        return true;
    }

//    if (found(KEYWORD, "debug_search")) {
//        parse_debug_search();
//        return true;
//    }

    // Every statement below this point invalidates the searcher
    m_searcher = nullptr;

    if (found(KEYWORD, "let")) {
        parse_definition();
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

    if (found(KEYWORD, "structure")) {
        parse_structure();
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

    return false;
}

void add_namespaces_with_children(std::unordered_set<Namespace *> &set, Namespace *space) {
    set.insert(space);
    for (const auto &entry: space->children())
        add_namespaces_with_children(set, entry.second.get());
}

void Parser::parse_open() {
    /*
        open NAMESPACE
     */

    consume(KEYWORD, "open");

    auto spaces = parse_namespace_collection();
    m_open_namespaces.insert(spaces.begin(), spaces.end());
}

void Parser::parse_close() {
    /*
        close NAMESPACE
     */

    consume(KEYWORD, "close");

    for (auto space: parse_namespace_collection()) {
        if (m_open_namespaces.count(space))
            m_open_namespaces.erase(space);
    }
}

#ifndef PATH_MAX
#define PATH_MAX (1024)
#endif

void Parser::parse_import() {
    /*
        import "PATH"
     */

    Token t_import = consume(KEYWORD, "import");
    std::string filename = consume(STRING).m_data;
    filename = filename.substr(1, filename.length() - 2); // removes surrounding (")

    // Convert path to normalized path
    std::string path = m_directory + filename;
    char absolute_path[PATH_MAX]; // PATH_MAX includes the \0 so +1 is not required
    char *result = realpath(path.c_str(), absolute_path);
    if (!result) {
        if (errno == ENOENT)
            throw ParserException(t_import, "could not find '" + filename + "'");
        else
            throw ParserException(t_import, "could not open '" + filename + "'");
    }
    path = absolute_path;

    // If file was already opened once, skip it
    if (m_imported_files->count(path))
        return;

    // Add the absolute path to list of imported files
    m_imported_files->insert(path);

    // Create input file stream
    std::ifstream ifstream(absolute_path);
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

void Parser::parse_definition() {
    /*
        let IDENTIFIER PARAMETERS : TYPE
     */

    consume(KEYWORD, "let");

    bool has_doc = (m_documentation != nullptr && m_last_comment_token.m_type == COMMENT);
    std::string doc;
    if (has_doc) doc = m_last_comment_token.m_data;

    // Optionally parse preference
    int preference = (found(SEPARATOR, "[")) ? parse_preference() : DEFAULT_PREFERENCE;
    for (auto &f: parse_functions(m_current_namespace->context(), {})) {
        // Store namespace in metadata (if not yet has one)
        if (!f->space())
            f->set_space(m_current_namespace);

        // Store preference
        if (preference != DEFAULT_PREFERENCE)
            m_current_namespace->set_preference(f, preference);

        // Store documentation
        if (has_doc) {
            auto path = to_path(*m_current_namespace, f->name());
            trim(m_last_comment_token.m_data);
            m_documentation->emplace(std::move(path), doc);
        }
    }
}

void Parser::parse_namespace() {
    /*
        namespace IDENTIFIER
            STATEMENT*
        end IDENTIFIER
     */

    consume(KEYWORD, "namespace");

    // Note: get_subspace creates a subspace if it does not yet exist
    auto previous = m_current_namespace;
    auto path = parse_path();
    m_current_namespace = &(m_current_namespace->get_subspace(path));

    while (parse_statement());

    consume(KEYWORD, "end");

    if (parse_path() != path)
        throw ParserException(m_current_token, "expected '" + path + "'");

    m_current_namespace = previous;
}

void Parser::parse_structure() {
    /*
        structure IDENTIFIER PARAMETERS := {
            FUNCTION , FUNCTION , ...
        }
     */

    Token t_structure = consume(KEYWORD, "structure");

    bool has_doc = (m_documentation != nullptr && m_last_comment_token.m_type == COMMENT);
    std::string doc;
    if (has_doc) doc = m_last_comment_token.m_data;

    auto identifier = consume(IDENTIFIER).m_data;

    // Parse parameters
    Context sub_context(m_current_namespace->context());
    Telescope parameters = parse_parameters(sub_context);

    consume(SEPARATOR, ":=");

    // Parse fields
    Telescope fields = parse_fields(sub_context, {});

    // Create a function for the structure type (e.g. `let Algebra  (k : Ring) : Type`)
    auto structure = Function::make(parameters, m_session.TYPE);
    // Create a constructor (e.g. `let Algebra.mk (k : Ring) (ring : Ring) (map : Morphism k ring) : Algebra k`)
    Telescope parameters_constructor = parameters;
    parameters_constructor.add(fields);
    auto constructor = Function::make(
            std::move(parameters_constructor),
            structure.specialize({}, parameters.functions())
    );

    // Set metadata
    structure->set_name(identifier);
    structure->set_constructor(constructor);
    structure->set_space(m_current_namespace);
    constructor->set_name(identifier + ".mk");

    // Put to context
    m_current_namespace->context().put(structure);

    // Store documentation
    if (has_doc) {
        auto path = to_path(*m_current_namespace, identifier);
        trim(m_last_comment_token.m_data);
        m_documentation->emplace(std::move(path), std::move(doc));
    }
}

void Parser::parse_check() {
    /*
        check EXPRESSION
     */

    consume(KEYWORD, "check");

    Formatter formatter;
    formatter.show_namespaces(m_options.show_namespaces);
    auto f = parse_expression(m_current_namespace->context(), {});
    auto str = formatter.format_definition(f);
    if (!f->is_base()) {
        str += " : ";
        str += formatter.format_expression(f.type());
    }

    if (m_options.json)
        output(Message::create(SUCCESS, str));
    else
        output("🦆 " + str);
}

void Parser::parse_search() {
    /*
        search LIST_OF_PARAMETERS
     */

    consume(KEYWORD, "search");

    // Parse telescope
    Context sub_context(m_current_namespace->context());
    Telescope telescope = parse_parameters(sub_context);
    std::vector<Telescope> groups = telescope.split(); // split for speed

    // Setup searcher
    setup_searcher();

    // Do a search, and store the results in an unordered_map by name
    std::unordered_map<std::string, FunctionRef> results;
    bool success = true;
    int query_counter = 0;
    auto start_time = std::chrono::system_clock::now();
    for (auto it = groups.begin(); success && it != groups.end(); ++it) {
        if (success &= m_searcher->search(*it)) {
            const auto &result = m_searcher->result();
            query_counter += m_searcher->counter();
            for (int i = 0; i < it->size(); ++i)
                results[it->functions()[i]->name()] = result[i];
        }
    }
    auto end_time = std::chrono::system_clock::now();

    // Print results in appropriate format
    if (success) {
        std::vector<FunctionRef> solutions;
        solutions.reserve(telescope.size());
        for (const auto &f: telescope.functions())
            solutions.push_back(results[f->name()]);
        output_search_results(telescope, solutions);
    } else {
        if (m_options.json)
            output(Message::create(SUCCESS, std::vector<std::string>()));
        else {
            output("🥺 no solutions found");
        }
    }

    CANARD_LOG("Search took " << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count()
                              << " ms (using " << query_counter << " queries)");

    // Clear searcher already (destructors take quite some time..)
    m_searcher->clear();
}

void Parser::parse_prove() {
    /*
        prove EXPRESSION
     */

    Token t_prove = consume(KEYWORD, "prove");

    auto f = parse_expression(m_current_namespace->context(), {});

    if (!f->is_base())
        throw ParserException(t_prove, "prove expects a base function");

    // Setup searcher
    setup_searcher();

    // Do a search, and store the results in a list
    auto start_time = std::chrono::system_clock::now();
    bool success = m_searcher->prove(f);
    auto end_time = std::chrono::system_clock::now();

    // Print results in appropriate format
    if (success) {
        output_search_results(Telescope({f}), m_searcher->result());
    } else {
        if (m_options.json)
            output(Message::create(SUCCESS, std::vector<std::string>()));
        else {
            output("🥺 no solutions found");
        }
    }

    // Clear searcher already (destructors take quite some time..)
    m_searcher->clear();

    CANARD_LOG("Search took " << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() << " ms");
}

void Parser::parse_inspect() {
    /*
        inspect NAMESPACE
    */

    consume(KEYWORD, "inspect");

    auto space = parse_absolute_namespace();

    // Construct list of identifiers
    Formatter formatter;
    formatter.show_namespaces(m_options.show_namespaces);
    std::vector<std::string> identifiers;
    for (auto &entry: space->context().map())
        identifiers.push_back(entry.first);

    // Output
    if (m_options.json)
        output(Message::create(SUCCESS, identifiers));
    else
        for (auto &identifier: identifiers)
            output(identifier);
}

void Parser::parse_docs() {
    /*
        docs EXPRESSION
    */

    consume(KEYWORD, "docs");

    auto f = parse_expression(m_current_namespace->context(), {});

    std::string doc;
    if (m_options.documentation && m_documentation != nullptr) {
        auto space = (Namespace *) f->space();
        if (space) {
            auto it = m_documentation->find(space->full_name() + '.' + f->name());
            if (it != m_documentation->end())
                doc = it->second;
        }
    }

    if (m_options.json)
        output(Message::create(SUCCESS, doc));
    else
        output("📝 " + (doc.empty() ? "No documentation" : doc));
}

void Parser::parse_debug_search() {
    /*
        debug_search LIST_OF_PARAMETERS
     */

    consume(KEYWORD, "debug_search");

    // Parse telescope
    Context sub_context(m_current_namespace->context());
    Telescope telescope = parse_parameters(sub_context);
    consume(SEPARATOR, ";");

    // Create DebugSearcher
    DebugSearcher searcher(std::make_shared<Query>(telescope));

    Formatter formatter;
    formatter.show_namespaces(m_options.show_namespaces);
    while (!searcher.query().is_solved()) {
        output("current query:\n" + formatter.format_query(searcher.query()));

        Context current_context(m_current_namespace->context());
        for (const auto &local_layer: searcher.query().locals()) {
            for (const auto &f: local_layer)
                current_context.put(f);
        }

        if (found(KEYWORD, "exit")) {
            consume();
            break;
        }

        auto thm = parse_expression(current_context, {});
        consume(SEPARATOR, ";");

        if (!searcher.apply(thm)) {
            output("failed to apply " + formatter.format_definition(thm));
            continue;
        }
    }

    if (!searcher.query().is_solved())
        return;

    // Print query tree
    output("Query tree:");
    output(formatter.format_query_tree(searcher.query()));

    // Print results
    auto result = searcher.query().final_solutions();
    output_search_results(telescope, result);
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

std::vector<FunctionRef> Parser::parse_functions(Context &context, const Telescope &parameters) {
    /*
            LIST_OF_IDENTIFIERS PARAMETERS : OUTPUT_TYPE
          | LIST_OF_IDENTIFIERS PARAMETERS := EXPRESSION
     */

    auto identifiers = parse_list_identifiers();

    // Parse parameters_full
    Context sub_context(context);
    Telescope parameters_full = parameters + parse_parameters(sub_context);
    Context &use_context = parameters_full.empty() ? context : sub_context;

    // Case declaration
    if (found(SEPARATOR, ":")) {
        Token t_colon = consume();
        auto type = parse_expression(use_context, {});
        // `type` must be a Type or a Prop, and is not allowed to have parameters
        if (type.type() != m_session.TYPE && type.type() != m_session.PROP)
            throw ParserException(t_colon, "expected a Type or Prop");
        if (!type->parameters().empty())
            throw ParserException(t_colon, "type of function cannot have parameters_full");
        // Create the functions
        std::vector<FunctionRef> output;
        for (const auto &identifier: identifiers) {
            // If the type has a constructor (i.e. comes from a structure), specialize the constructor
            if (type->constructor() != nullptr) {
                const auto &constructor = type->constructor();
                // The fields of the structure are encoded in the parameters_full of the constructor, cloning them gives us fields of a new instance of the structure
                std::unique_ptr<Matcher> sub_matcher;
                Telescope fields = Matcher::dummy().clone(parameters_full, constructor->parameters(), &sub_matcher);
                // Update names
                for (auto &g: fields.functions()) {
                    g->set_name(identifier + '.' + g->name());
                    g->set_implicit(true);
                    if (!context.put(g))
                        throw ParserException(m_current_token, "identifier '" + g->name() + "' already used in this context");
                    output.push_back(g);
                }
                // Construct structure instance by specializing the constructor
                auto f = constructor.specialize(parameters_full, sub_matcher->convert(constructor->parameters().functions()));
                f->set_name(identifier);
                if (!context.put(f))
                    throw ParserException(m_current_token, "identifier '" + f->name() + "' already used in this context");
                output.push_back(std::move(f));
            }
                // If no constructor, just make a function in the regular way
            else {
                // Note: don't move parameters_full, since they are used multiple times
                auto f = Function::make(parameters_full, type);
                f->set_name(identifier);
                if (!context.put(f))
                    throw ParserException(t_colon, "identifier '" + identifier + "' already used in this context");
                output.push_back(std::move(f));
            }
        }
        return output;
    }

    // Case definition
    if (found(SEPARATOR, ":=")) {
        Token t_def = consume();

        // Only makes sense to have a single
        if (identifiers.size() != 1)
            throw ParserException(t_def, "definition does not allow multiple identifiers");
        const auto &identifier = identifiers[0];

        // Parse and return expression
        return {parse_expression(use_context, parameters_full, &context, &identifier)};
    }

    {
        std::ostringstream ss;
        ss << "unexpected token " << Lexer::to_string(m_current_token.m_type) << " " << m_current_token.m_data << ", expected : or :=";
        throw ParserException(m_current_token, ss.str());
    }
}

Telescope Parser::parse_parameters(Context &context) {
    Telescope parameters;
    std::vector<FunctionRef> implicits;
    bool is_implicit;
    while ((is_implicit = found(SEPARATOR, "{")) || found(SEPARATOR, "(")) {
        consume();
        for (const auto &f: parse_functions(context, {})) {
            f->set_implicit(is_implicit || f->implicit());
            parameters.add(f);
            if (f->implicit()) implicits.push_back(f);
        }
        consume(SEPARATOR, is_implicit ? "}" : ")");
    }

    // Shortcut
    if (implicits.empty())
        return parameters;

    // Re-order implicit parameters so that every implicit parameter can be inferred from the first explicit parameter following it
    // To do this, first make a copy of the parameters to match with
    Telescope copy = Matcher::dummy().clone({}, parameters);

    // Now reorder the parameters
    Telescope reordered;
    Matcher matcher(parameters.functions());
    const size_t n = parameters.size();
    for (int i = 0; i < n; ++i) {
        const auto &f = parameters.functions()[i];
        if (f->implicit())
            continue;

        matcher.assert_matches(f, copy.functions()[i]);

        // Add all implicits that can be inferred now
        for (auto it_implicits = implicits.begin(); it_implicits != implicits.end();) {
            const auto &g = *it_implicits;
            if (matcher.has_solution(g)) {
                reordered.add(g);
                it_implicits = implicits.erase(it_implicits);
            } else {
                ++it_implicits;
            }
        }
        // Then add the explicit parameter afterwards
        reordered.add(f);
    }

    // There should be no implicit parameters left at this point
    if (!implicits.empty()) {
        Formatter formatter;
        formatter.show_namespaces(m_options.show_namespaces);
        throw ParserException(m_current_token, "implicit parameter " + formatter.format_definition(implicits[0]) + " cannot be inferred");
    }

    return reordered;
}

Telescope Parser::parse_fields(Context &context, const Telescope &parameters) {
    /*
        { FUNCTIONS ( , FUNCTIONS )*  }
     */

    consume(SEPARATOR, "{");

    Telescope fields;
    while (true) {
        fields.add(parse_functions(context, parameters));
        if (found(SEPARATOR, ",")) {
            consume();
            continue;
        }
        break;
    }
    consume(SEPARATOR, "}");

    return fields;
}

FunctionRef Parser::parse_expression(Context &context, const Telescope &parameters, Context *store_context, const std::string *identifier) {
    /*
        EXPRESSION = TERM | TERM TERM+ | EXPRESSION { STRUCTURE_ARGUMENTS }
     */

    // Lambda expressions: (λ or \) PARAMETERS := EXPRESSION
    if (found(SEPARATOR, "\\") || found(SEPARATOR, "λ")) {
        consume();
        Context lambda_context(context);
        Telescope lambda_parameters = parse_parameters(lambda_context);
        consume(SEPARATOR, ":=");
        // Combine parameters with the λ-parameters
        return parse_expression(lambda_context, parameters + lambda_parameters, store_context, identifier);
    }

    // Parse an initial term
    FunctionRef f = parse_term(context);
    if (f == nullptr)
        throw ParserException(m_current_token, "expected expression but not found");

    // Parse trailing terms
    std::vector<FunctionRef> trailing;
    for (auto term = parse_term(context); term != nullptr; term = parse_term(context))
        trailing.push_back(term);

    // If there are trailing terms, or there are parameters, we need to specialize
    const size_t n = trailing.size();
    if (n > 0 || !parameters.empty()) {
        std::vector<FunctionRef> arguments;
        auto it_trailing = trailing.begin();
        const auto m = f->parameters().size();
        // (sorry for the tricky for-loop: meant to range over first n explicit parameters of `initial`, but m != n necessarily)
        for (int i = 0; i < (int) m && it_trailing != trailing.end(); ++i) {
            // Insert `nullptr` for each implicit argument.
            if (f->parameters().functions()[i]->implicit())
                arguments.emplace_back(nullptr);
            else
                arguments.push_back(std::move(*it_trailing++));
        }
        // Specialize f
        try {
            f = f.specialize(parameters, std::move(arguments));
        } catch (SpecializationException &e) {
            throw ParserException(m_current_token, format_specialization_exception(e));
        }
    }

    // Parse structure fields
    if (found(SEPARATOR, "{")) {
        // Make sure f has a constructor
        if (!f->constructor()) {
            Formatter formatter;
            formatter.show_namespaces(m_options.show_namespaces);
            throw ParserException(m_current_token, formatter.format_definition(f) + " does not have a constructor");
        }
        // Clone the constructor
        FunctionRef constructor = (f->constructor()->is_base()) ? f->constructor() : Matcher::dummy().clone(f->constructor());
        // Parse the fields
        Context sub_context(context);
        auto fields = parse_fields(sub_context, parameters);
        // Determine constructor arguments
        std::vector<FunctionRef> constructor_arguments = parameters.functions();
        for (auto it = constructor->parameters().functions().begin() + (int) parameters.size();
             it != constructor->parameters().functions().end(); ++it) { // sorry for the ugly for-loop
            const auto &field_name = (*it)->name();
            const auto &value = sub_context.get(field_name);
            if (value == nullptr)
                throw ParserException(m_current_token, "missing field '" + field_name + "'");
            constructor_arguments.push_back(value.specialize({}, parameters.functions()));
            // Store fields in context as well
            if (store_context && identifier) {
                std::string value_identifier = *identifier + '.';
                value_identifier += field_name;
                if (!store_context->put(value_identifier, value))
                    throw ParserException(m_current_token, "identifier '" + value_identifier + "' already used in this context");
            }
        }
        // Specialize the constructor
        f = constructor.specialize(parameters, constructor_arguments);
    }
    // Set name of f and store in context
    if (store_context && identifier) {
        // Note: only if f does not have a name yet, it is 'safe' to give it one. Otherwise, we might be overwriting an existing name!
        if (f->name().empty())
            f->set_name(*identifier);
        if (!store_context->put(*identifier, f)) {
            throw ParserException(m_current_token, "identifier '" + *identifier + "' already used in this context");
        }
    }

    return f;
}

FunctionRef Parser::parse_term(Context &context) {
    /*
        TERM = PATH | ( EXPRESSION )
     */

    if (found(SEPARATOR, "(")) {
        consume();
        auto f = parse_expression(context, {});
        consume(SEPARATOR, ")");
        return f;
    }

    if (found(IDENTIFIER)) {
        Token token = m_current_token;
        std::string path = parse_path();
        // (1) Look through the context
        auto f = context.get(path);
        if (f != nullptr)
            return f;

        // (2) Look through namespaces from the current one to the top
        for (auto space = m_current_namespace; space != nullptr; space = space->parent()) {
            f = space->get_function(path);
            if (f != nullptr)
                return f;
        }

        // (3) Look through the other open namespaces
        std::vector<FunctionRef> candidates;
        for (auto &space: m_open_namespaces) {
            f = space->get_function(path);
            if (f != nullptr) candidates.push_back(f);
        }
        // If there is a unique candidate, we are good
        if (candidates.size() == 1)
            return candidates[0];
        // If more than one candidate, its ambiguous
        if (candidates.size() > 1) {
            Formatter formatter;
            formatter.show_namespaces(true);
            std::ostringstream ss;
            for (int i = 0; i < (int) candidates.size(); ++i) {
                if (i == (int) candidates.size() - 1) ss << " or ";
                ss << formatter.format_identifier(candidates[i]);
                if (i < (int) candidates.size() - 2) ss << ", ";
            }
            throw ParserException(token, "ambiguous identifier '" + path + "', could be " + ss.str());
        }
        // (4) Otherwise, if no candidates, throw unknown
        throw ParserException(token, "unknown identifier '" + path + "'");
    }

    return nullptr;
}

Namespace *Parser::parse_absolute_namespace() {
    Token t_path = m_current_token;
    std::string path = parse_path();
    auto space = m_session.global_namespace().find_subspace(path);
    if (space == nullptr)
        throw ParserException(t_path, "no namespace '" + path + "'");
    return space;
}

std::unordered_set<Namespace *> Parser::parse_namespace_collection() {
    std::ostringstream ss;
    bool asterisk = false;
    while (true) {
        if (found(SEPARATOR, "*")) {
            consume();
            asterisk = true;
            break;
        }

        if (found(IDENTIFIER))
            ss << consume().m_data;

        if (!found(SEPARATOR, "."))
            break;

        consume();

        if (!found(SEPARATOR, "*"))
            ss << ".";
    }

    const auto path = ss.str();
    const auto space = m_session.global_namespace().find_subspace(path);
    if (space == nullptr)
        throw ParserException(m_current_token, "no namespace '" + path + "'");

    // If an asterisk, return all children
    if (asterisk) {
        std::unordered_set<Namespace *> spaces;
        space->get_all_subspaces(spaces);
        return spaces;
    }

    // Otherwise, return a singleton
    return {space};
}

int Parser::parse_preference() {
    /*
        [ NUMBER % ]
     */

    consume(SEPARATOR, "[");
    const int percentage = std::stoi(consume(NUMBER).m_data);
    consume(SEPARATOR, "%");
    consume(SEPARATOR, "]");
    return percentage;
}

void Parser::setup_searcher() {
    // If already a searcher, nothing to do
    if (m_searcher != nullptr)
        return;
    // Create set of namespaces whose theorems to use
    std::unordered_set<Namespace *> spaces = m_open_namespaces;
    for (auto space = m_current_namespace; space != nullptr; space = space->parent())
        spaces.insert(space);
    // Create searcher
    m_searcher = std::unique_ptr<Searcher>(new Searcher(spaces, m_options.max_search_depth, m_options.max_search_threads));
}

void Parser::output(const std::string &message) {
    m_ostream << message << std::endl;
}

void Parser::output_search_results(const Telescope &telescope, const std::vector<FunctionRef> &result) {
    Formatter formatter;
    formatter.show_namespaces(m_options.show_namespaces);
    const auto n = telescope.functions().size();
    std::vector<std::string> keys, values;
    keys.reserve(n);
    values.reserve(n);
    for (int i = 0; i < n; ++i) {
        const auto &f = telescope.functions()[i];
        if (!f->implicit()) {
            keys.push_back(f->name());
            values.push_back(formatter.format_expression(result[i]));
        }
    }
    if (m_options.json) {
        output(Message::create(SUCCESS, keys, values));
    } else {
        std::stringstream ss;
        ss << "🔎 ";
        const auto m = keys.size();
        bool first = true;
        for (int i = 0; i < m; ++i) {
            if (!first) ss << ", ";
            first = false;
            ss << keys[i] << " = " << values[i];
        }
        output(ss.str());
    }
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

void Parser::set_documentation(std::unordered_map<std::string, std::string> *documentation) {
    m_documentation = documentation;
}

std::string Parser::format_specialization_exception(SpecializationException &exception) const {
    Formatter formatter;
    formatter.show_namespaces(m_options.show_namespaces);

    std::string message = exception.m_message;
    auto it_f = message.find("{f}");
    if (it_f != std::string::npos)
        message = message.replace(it_f, 3, formatter.format_definition(exception.m_map.at("f")));
    auto it_g = message.find("{g}");
    if (it_g != std::string::npos)
        message = message.replace(it_g, 3, formatter.format_definition(exception.m_map.at("g")));
    return message;
}
