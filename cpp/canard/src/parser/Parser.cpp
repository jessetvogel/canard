//
// Created by Jesse Vogel on 01/07/2021.
//

#include "Parser.h"
#include "Message.h"
#include "../searcher/Searcher.h"
#include "../core/macros.h"
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

std::string to_path(const Namespace &space, const std::string &name) {
    return space.name().empty() ? name : space.full_name() + '.' + name;
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
    }
//    catch (std::exception &e) {
//        error("Exception: " + std::string(e.what()));
//        return false;
//    }
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
    auto space = m_session.get_global_namespace().get_namespace(path); // TODO: resolve_namespace ? (where resolve means also asking parents)
    if (space == nullptr)
        throw ParserException(t_inspect, "unknown namespace '" + path + "'");

    // Construct list of identifiers
    Formatter formatter;
    formatter.show_namespaces(m_options.show_namespaces);
    std::vector<std::string> identifiers;
    for (auto &entry: space->context().functions())
        identifiers.push_back(entry.first);

    // Output
    if (m_options.json)
        output(Message::create(SUCCESS, identifiers));
    else
        for (auto &identifier: identifiers)
            output(identifier);
}

void Parser::parse_doc() {
    /*
        doc IDENTIFIER
    */

    Token t_doc = consume(KEYWORD, "doc");
    auto path = parse_path();

    std::string doc;
    if (m_options.documentation && m_documentation != nullptr) {
        auto it = m_documentation->find(path); // TODO: first find thing, and then convert to full path ?
        if (it != m_documentation->end())
            doc = it->second;
    }

    if (m_options.json)
        output(Message::create(SUCCESS, doc));
    else
        output("📝 " + (doc.empty() ? "No documentation" : doc));
}

void add_namespaces_with_children(std::unordered_set<Namespace *> &set, Namespace *space) {
    set.insert(space);
    for (auto child: space->children())
        add_namespaces_with_children(set, child);
}

void Parser::parse_open() {
    /*
        open NAMESPACE
     */

    Token t_open = consume(KEYWORD, "open");

    // `open *` opens all available namespaces
    if (found(SEPARATOR, "*")) {
        consume();
        for (auto space: m_session.get_global_namespace().children())
            add_namespaces_with_children(m_open_namespaces, space);
        return;
    }

    // Otherwise, open by identifier
    std::string path = parse_path();
    auto space = m_session.get_global_namespace().get_namespace(path);
    if (space == nullptr)
        throw ParserException(t_open, "no namespace '" + path + "'");
    m_open_namespaces.insert(space);
}

void Parser::parse_close() {
    /*
        close NAMESPACE
     */

    Token t_close = consume(KEYWORD, "close");

    // `close *` closes all namespaces
    if (found(SEPARATOR, "*")) {
        consume();
        m_open_namespaces.clear();
        return;
    }

    // Otherwise, close by identifier
    std::string path = parse_path();
    auto space = m_session.get_global_namespace().get_namespace(path);
    if (space == nullptr)
        throw ParserException(t_close, "no namespace '" + path + "'");
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
    // TODO: WTF is this ?
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
    std::string identifier = consume(IDENTIFIER).m_data;
    // If namespace already exists, use that one. Otherwise, create a new one
    auto space = m_current_namespace->get_namespace(identifier);
    m_current_namespace = (space != nullptr) ? space : m_current_namespace->create_subspace(identifier);

    while (parse_statement());

    consume(KEYWORD, "end");
    consume(IDENTIFIER, identifier);
    m_current_namespace = m_current_namespace->parent();
}

void Parser::parse_structure() {
    /*
        structure IDENTIFIER PARAMETERS := {
            FUNCTION , FUNCTION , ...
        }
     */

    // TODO: handle documentation as well

    Token t_structure = consume(KEYWORD, "structure");

    auto identifier = consume(IDENTIFIER).m_data;

    // Parse parameters
    Context sub_context(m_current_namespace->context());
    Telescope parameters = parse_parameters(sub_context);

    consume(SEPARATOR, ":=");
    consume(SEPARATOR, "{"); // TODO: is this also just `parse_fields` ?

    std::vector<FunctionRef> fields;
    while (true) {
        // Add fields (TODO: more efficient way of extending a vector ?)
        for (auto &f: parse_functions(sub_context))
            fields.push_back(f);

        // Continue if there is a comma
        if (found(SEPARATOR, ",")) {
            consume();
            continue;
        }
        break;
    }

    consume(SEPARATOR, "}");

    // Create a function for the structure type (e.g. `let Algebra  (k : Ring) : Type`)
    auto structure = Function::make(parameters, m_session.TYPE);
    // Create a constructor (e.g. `let Algebra.mk (k : Ring) (ring : Ring) (map : Morphism k ring) : Algebra k`)
    Telescope parameters_constructor = parameters;
    for (const auto &f: fields)
        parameters_constructor.add(f);
    auto constructor = Function::make(
            std::move(parameters_constructor),
            structure.specialize({}, parameters.functions())
    );

    // Set metadata
    auto metadata = std::make_shared<Metadata>();
    metadata->m_space = m_current_namespace;
    metadata->m_constructor = constructor;
    structure->set_name(identifier);
    structure->set_metadata(std::move(metadata));
    auto metadata_constructor = std::make_shared<Metadata>();
    constructor->set_metadata(std::move(metadata_constructor));
    constructor->set_name(identifier + ".mk");

    // Put to context
    m_current_namespace->context().put(structure);
}

void Parser::parse_search() {
    /*
        search LIST_OF_PARAMETERS
     */

    consume(KEYWORD, "search");

    // Parse indeterminates
    std::vector<FunctionRef> indeterminates;
    Context sub_context(m_current_namespace->context());
    while (found(SEPARATOR, "(")) {
        consume();
        auto functions = parse_functions(sub_context);
        indeterminates.insert(indeterminates.end(), functions.begin(), functions.end());
        consume(SEPARATOR, ")");
    }

    // Create searcher and specify the searching space
    Searcher searcher(m_options.max_search_depth, m_options.max_search_threads);
    std::unordered_set<Namespace *> added_namespaces;
    for (auto space = m_current_namespace; space != nullptr; space = space->parent()) {
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

    Formatter formatter;
    formatter.show_namespaces(m_options.show_namespaces);

    if (m_options.json) {
        if (success) {
            std::vector<std::string> keys, values;
            keys.reserve(indeterminates.size());
            values.reserve(result.size());
            for (auto &f: indeterminates)
                keys.push_back(f->name());
            for (auto &g: result)
                values.push_back(formatter.to_string(g));
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
            ss << indeterminates[i]->name() << " = " << formatter.to_string(result[i]);
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

    Formatter formatter;
    formatter.show_namespaces(m_options.show_namespaces);
    auto f = parse_expression(m_current_namespace->context());
    auto str = formatter.to_string_full(f);

    if (m_options.json)
        output(Message::create(SUCCESS, str));
    else
        output("🦆 " + str);
}

void Parser::parse_declaration() {
    /*
        let IDENTIFIER PARAMETERS : TYPE
     */

    consume(KEYWORD, "let");

    bool has_doc = (m_documentation != nullptr && m_comment_token.m_type == COMMENT);

    for (auto &f: parse_functions(m_current_namespace->context())) {
        // Store namespace in metadata (for base functions)
        if (f->is_base()) {
            auto metadata = (Metadata *) f->metadata();
            if (metadata) metadata->m_space = m_current_namespace;
        }

        // Store documentation
        if (has_doc) {
            auto path = to_path(*m_current_namespace, f->name());
            m_documentation->emplace(std::move(path), m_comment_token.m_data);
        }
    }
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

std::vector<FunctionRef> Parser::parse_functions(Context &context) {
    /*
            LIST_OF_IDENTIFIERS PARAMETERS : OUTPUT_TYPE
          | LIST_OF_IDENTIFIERS PARAMETERS := EXPRESSION
     */

    auto identifiers = parse_list_identifiers();

    // Parse parameters
    Context sub_context(context);
    Telescope parameters = parse_parameters(sub_context);
    Context &use_context = parameters.empty() ? context : sub_context;

    // Case declaration
    if (found(SEPARATOR, ":")) {
        Token t_colon = consume();
        auto type = parse_expression(use_context);

        if (type.type() != m_session.TYPE && type.type() != m_session.PROP)
            throw ParserException(t_colon, "expected a Type or Prop");
        // TODO: might omit this condition at some point, instead just concat the parameters!
        if (!type->parameters().empty())
            throw ParserException(t_colon, "type of function cannot have parameters");

        auto &type_metadata = *(Metadata *) type->metadata();

        // Create the functions
        std::vector<FunctionRef> output;
        for (auto &identifier: identifiers) {
            // If the type has a constructor (i.e. comes from a structure)
            if (type_metadata.m_constructor != nullptr) {
                // TODO: this assumes the constructor has no parameters other than the fields, is that right?
                //  I believe so as types are not allowed to have parameters.
                //  If this ever changes, move the parameters of the type to the parameters of the function before this code.
                const auto &constructor = type_metadata.m_constructor;
                // The fields of the structure are encoded in the parameters of the constructor, cloning them gives us fields of a new instance of the structure
                Matcher matcher({});
                std::unique_ptr<Matcher> sub_matcher;
                Telescope fields = matcher.clone(parameters, constructor->parameters(), &sub_matcher);
                // Update names
                for (auto &g: fields.functions()) {
                    auto metadata = std::make_shared<Metadata>();
                    metadata->m_implicit = true;
                    g->set_metadata(std::move(metadata));
                    g->set_name(identifier + '.' + g->name());
                    if (!context.put(g))
                        throw ParserException(m_current_token, "failed to add " + g->name() + " to context");
                    output.push_back(g);
                }
                // Construct structure instance by specializing the constructor
                auto f = constructor.specialize(parameters, sub_matcher->convert(constructor->parameters().functions()));
                f->set_name(identifier);
                if (!context.put(f))
                    throw ParserException(m_current_token, "failed to add " + f->name() + " to context");
                output.push_back(std::move(f));
            }
                // If no constructor, just make a function in the regular way
            else {
                // Note: don't move parameters, since they are used multiple times
                auto f = Function::make(parameters, type);
                f->set_metadata(std::make_shared<Metadata>());
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
            throw ParserException(t_def, "Multiple identifiers doesn't make sense here!");
        const auto &identifier = identifiers[0];

        // Parse expression
        auto f = parse_expression(use_context, parameters);

        // TODO: maybe here we should parse structure fields ?

        // Note: only if f does not have a name yet, it is 'safe' to give it one. Otherwise, we might be overwriting an existing name!
        if (f->name().empty())
            f->set_name(identifier);
        if (!context.put(identifier, f))
            throw ParserException(t_def, "identifier '" + identifier + "' already used in this context");

        return {f};
    }

    throw ParserException(m_current_token, "unexpected token");
}

Telescope Parser::parse_parameters(Context &context) {
    Telescope parameters;
    std::vector<FunctionRef> implicits;
    bool is_implicit;
    while ((is_implicit = found(SEPARATOR, "{")) || found(SEPARATOR, "(")) {
        consume();
        for (const auto &f: parse_functions(context)) {
            auto f_metadata = (Metadata *) f->metadata();
            if (f_metadata) f_metadata->m_implicit |= is_implicit;

            parameters.add(f);

            if (f_metadata && f_metadata->m_implicit) implicits.push_back(f);
        }
        consume(SEPARATOR, is_implicit ? "}" : ")");
    }

    // Shortcut
    if (implicits.empty())
        return parameters;

    // TODO: This seems very slow.. Can this be optimized?
    // Re-order implicit parameters so that every implicit parameter can be inferred from the first explicit parameter following it
    // To do this, first make a copy of the parameters to match with
    Matcher dummy({});
    Telescope copy = dummy.clone({}, parameters);

    // Now reorder the parameters
    Telescope reordered;
    Matcher matcher(parameters.functions());
    const size_t n = parameters.size();
    for (int i = 0; i < n; ++i) {
        const auto &f = parameters.functions()[i];
        if (f->metadata() && ((Metadata *) f->metadata())->m_implicit)
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
    if (!implicits.empty())
        throw ParserException(m_current_token, "implicit parameter '" + implicits[0]->name() + "' cannot be inferred");

    return reordered;
}

FunctionRef Parser::parse_expression(Context &context) {
    return parse_expression(context, {});
}

std::string Parser::format_specialization_exception(SpecializationException &exception) const {
    Formatter formatter;
    formatter.show_namespaces(m_options.show_namespaces);

    std::string message = exception.m_message;
    auto it_f = message.find("{f}");
    if (it_f != std::string::npos)
        message = message.replace(it_f, 3, formatter.to_string_full(exception.m_f));
    auto it_g = message.find("{g}");
    if (it_g != std::string::npos)
        message = message.replace(it_g, 3, formatter.to_string_full(exception.m_g));
    return message;
}

FunctionRef Parser::parse_expression(Context &context, const Telescope &parameters) {
    /*
        EXPRESSION = TERM | TERM TERM+ | EXPRESSION { STRUCTURE_ARGUMENTS }
     */

    // Support for lambda expressions: (λ or \) PARAMETERS -> EXPRESSION
    if (found(SEPARATOR, "\\") || found(SEPARATOR, "λ")) {
        consume();
        Context lambda_context(context);
        Telescope lambda_parameters = parse_parameters(lambda_context);
        consume(SEPARATOR, ":=");
        // Combine parameters with the λ-parameters
        return parse_expression(lambda_context, parameters + lambda_parameters);
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
            auto parameter_metadata = (Metadata *) f->parameters().functions()[i]->metadata();
            // Insert `nullptr` for each implicit argument.
            if (parameter_metadata && parameter_metadata->m_implicit)
                arguments.emplace_back(nullptr);
            else
                arguments.push_back(std::move(*it_trailing++));
        }
        try {
            auto metadata = std::make_shared<Metadata>();
            // If initial has a constructor (i.e. is a structure), also specialize the constructor
            auto f_metadata = (Metadata *) f->metadata();
            if (f_metadata && f_metadata->m_constructor != nullptr)
                metadata->m_constructor = f_metadata->m_constructor.specialize(parameters, arguments);
            // Specialize f
            f = f.specialize(parameters, std::move(arguments));
            f->set_metadata(std::move(metadata));
        } catch (SpecializationException &e) {
            throw ParserException(m_current_token, format_specialization_exception(e));
        }
    }

//    // Parse structure fields
//    if (found(SEPARATOR, "{")) {
//        // Make sure f has a constructor
//        auto metadata = (Metadata *) f->metadata();
//        if (!metadata || !metadata->m_constructor)
//            throw ParserException(m_current_token, "trying to create structure instance without constructor");
//        const auto &constructor = metadata->m_constructor;
//        // Parse the fields
//        Context &sub_context(context);
//        Matcher matcher({});
//        std::unique_ptr<Matcher> sub_matcher;
//        std::unordered_map<std::string, FunctionRef> fields;
//        for (const auto &g: matcher.clone(parameters, parse_fields(sub_context), &sub_matcher).functions())
//            fields.emplace(g->name(), g);
//
//        std::vector<FunctionRef> constructor_arguments = parameters.functions();
//        for ()
//            constructor_arguments.push_back(fields.find(...));
//        f = constructor.specialize(parameters, constructor_arguments)
//
//        // TODO: biggest problem is: how do we give the fields the proper names/identifiers and put them in the context ?
//        //  maybe this part should actually be done in the parse_functions method ??
//    }

    return f;
}

Telescope Parser::parse_fields(Context &context) {
    /*
        { FUNCTIONS ( , FUNCTIONS )*  }
     */

    if (!found(SEPARATOR, "{"))
        return {};

    consume();

    Telescope fields;
    while (true) {
        for (auto &field: parse_functions(context))
            fields.add(field);
        // Continue if there is a comma
        if (found(SEPARATOR, ",")) {
            consume();
            continue;
        }
        break;
    }
    consume(SEPARATOR, "}");

    return fields;
}

FunctionRef Parser::parse_term(Context &context) {
    /*
        TERM = PATH | ( EXPRESSION )
     */

    if (found(SEPARATOR, "(")) {
        consume();
        auto f = parse_expression(context);
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
        f = m_current_namespace->resolve_function(path);
        if (f != nullptr)
            return f;

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
                ss << formatter.to_string(candidates[i]);
                if (i < (int) candidates.size() - 2) ss << ", ";
            }
            throw ParserException(token, "ambiguous identifier '" + path + "', could be " + ss.str());
        }
        // (4) Otherwise, if no candidates, throw unknown
        throw ParserException(token, "unknown identifier '" + path + "'");
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

void Parser::set_documentation(std::unordered_map<std::string, std::string> *documentation) {
    m_documentation = documentation;
}
