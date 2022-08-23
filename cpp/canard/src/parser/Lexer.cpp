//
// Created by Jesse Vogel on 01/07/2021.
//

#include <algorithm>
#include "Lexer.h"

const std::vector<std::string> &Lexer::keywords() {
    static const std::vector<std::string> KEYWORDS(
            {
                    "let", "check", "search", "import", "namespace", "end", "open", "close", "structure",
                    "exit", "inspect", "docs", "debug_search", "prove"
            });
    return KEYWORDS;
}

const std::vector<std::string> &Lexer::separators() {
    static const std::vector<std::string> SEPARATORS(
            {
                    ":", "(", ")", "{", "}", "_", ";", ".", ":=", "--", "*", "\\", "λ", "->", ","
            });
    return SEPARATORS;
}

const char *Lexer::to_string(const TokenType &type) {
    switch (type) {
        case NONE:
            return "NONE";
        case IDENTIFIER:
            return "IDENTIFIER";
        case SEPARATOR:
            return "SEPARATOR";
        case KEYWORD:
            return "KEYWORD";
        case NEWLINE:
            return "NEWLINE";
        case NUMBER:
            return "NUMBER";
        case STRING:
            return "STRING";
        case END_OF_FILE:
            return "END_OF_FILE";
        case COMMENT:
            return "COMMENT";
    }
}

bool Lexer::is_keyword(const std::string &str) {
    return std::find(keywords().begin(), keywords().end(), str) != keywords().end();
}

bool Lexer::is_separator(const std::string &str) {
    return std::find(separators().begin(), separators().end(), str) != separators().end();
}

bool Lexer::is_number(const std::string &str) {
    for (char c: str)
        if (!(c >= '0' && c <= '9'))
            return false;
    return true;
}

bool Lexer::is_word(const std::string &str) {
    for (char c: str)
        if (!(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') && c != '_' && !(c >= '0' && c <= '9'))
            return false;
    return true;
}

bool Lexer::is_string(const std::string &str) {
    const size_t n = str.length();
    return (n >= 2 && str[0] == '"' && str[n - 1] == '"');
}

bool Lexer::is_newline(const std::string &str) {
    const size_t n = str.length();
    return (n == 1 && str[0] == '\n') || (n == 2 && str[0] == '\r' && str[1] == '\n');
}

Lexer::Lexer(Scanner &scanner) : m_scanner(scanner) {}

Token Lexer::get_token() {
    // If there are tokens in the FIFO queue, use those first
    if (!m_queue.empty()) {
        Token token = std::move(m_queue.front());
        m_queue.pop();
        return token;
    }

    // Read characters until a new Token is produced
    while (true) {
        int c = m_scanner.get_char();

        // End of file
        if (c == -1) {
            if (m_stream_length == 0)
                return {END_OF_FILE, std::string(), m_scanner.line(), m_scanner.position()};
            if (!tokenize(m_stream.str()))
                throw LexerException(m_current_token.m_line, m_current_token.m_position, "unexpected end of file");
            return make_token();
        }

        // Whitespace: always marks the end of a get_token (if there currently is one)
        if (c == ' ' || c == '\t') {
            if (m_stream_length != 0)
                return make_token();
            continue;
        }

        // Line comments (--): mark the end of a get_token (if there currently is one),
        // then continue discarding characters until a newline appears
        if (c == '-' && m_scanner.get_previous_char() == '-') {
            // Remove the first '-'
            stream_remove_back();

            // If the stream is non-empty, force a token
            if (m_stream_length > 0)
                m_queue.push(make_token());

            // Read until a newline or end-of-file
            while (true) {
                c = m_scanner.get_char();
                if (c != -1 && c != '\n')
                    m_stream << (char) c;
                else
                    break;
            }

            // Create a comment Token
            m_queue.push({COMMENT, m_stream.str(), m_current_token.m_line, m_current_token.m_position});
            reset_token();

            // Include the newlines if it follows
            if (c == '\n')
                m_queue.push({NEWLINE, "\n", m_current_token.m_line, m_current_token.m_position});

            return get_token();
        }

        // Block comments (/- * -/): do a similar thing as line comments
        if (c == '-' && m_scanner.get_previous_char() == '/') {
            // Remove the '/' from the sb before making a get_token!
            stream_remove_back();

            // If the stream is non-empty, force a token
            if (m_stream_length > 0)
                m_queue.push(make_token());

            // Read until a newline or end-of-file
            while (true) {
                c = m_scanner.get_char();
                if (c == -1)
                    throw LexerException(m_scanner.line(), m_scanner.position(), "unexpected end of file");
                if (!(m_scanner.get_previous_char() == '-' && c == '/'))
                    m_stream << (char) c;
                else
                    break;
            }

            // Create a comment Token
            std::string comment = m_stream.str();
            comment.pop_back(); // remove the final '-'
            m_queue.push({COMMENT, std::move(comment), m_current_token.m_line, m_current_token.m_position});
            reset_token();

            return get_token();
        }

        // Enlarge the get_token if possible
        m_stream << (char) c;
        m_stream_length++;

        // If can tokenize, just continue
        if (tokenize(m_stream.str()))
            continue;

        // If we also did not tokenize before, hope that it will make sense later
        if (m_current_token.m_type == NONE)
            continue;

        // Return the last valid get_token
        Token token = make_token();
        m_stream << (char) c;
        ++m_stream_length;
        tokenize(m_stream.str());
        return token;
    }
}

bool Lexer::tokenize(std::string str) {
    TokenType type;
    if (is_keyword(str))
        type = KEYWORD;
    else if (is_separator(str))
        type = SEPARATOR;
    else if (is_newline(str))
        type = NEWLINE;
    else if (is_number(str))
        type = NUMBER;
    else if (is_word(str))
        type = IDENTIFIER;
    else if (is_string(str)) {
        type = STRING;
//        str = str.substring(1, str.length() - 1);
    } else
        return false;

    // So that there are not constantly made new instances of Token
    m_current_token.m_type = type;
    m_current_token.m_data = std::move(str);
    return true;
}

Token Lexer::make_token() {
    if (m_current_token.m_type == NONE) {
        std::string message = "unrecognized token '" + m_stream.str() + "'";
        m_stream.str(std::string());
        m_stream_length = 0;
        throw LexerException(m_current_token.m_line, m_current_token.m_position, message);
    }

    Token token = m_current_token;
    reset_token();
    return token;
}

void Lexer::reset_token() {
    m_current_token.m_type = NONE;
    m_current_token.m_line = m_scanner.line(); // TODO: set these correctly at the correct place
    m_current_token.m_position = m_scanner.position();
    m_stream.str(std::string());
    m_stream_length = 0;
}

void Lexer::stream_remove_back() {
    // Remove the last character and update length
    m_stream.seekp(-1, m_stream.cur);
    --m_stream_length;
}
