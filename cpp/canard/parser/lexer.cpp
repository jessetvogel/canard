//
// Created by Jesse Vogel on 01/07/2021.
//

#include <algorithm>
#include "lexer.h"

const std::vector<std::string> Lexer::KEYWORDS(
        {"let", "def", "check", "search", "import", "namespace", "end", "open", "close", "exit", "inspect"});
const std::vector<std::string> Lexer::SEPARATORS({":", "(", ")", "{", "}", "_", ";", ".", ":=", "--"});

bool Lexer::is_keyword(std::string &str) {
    return std::find(KEYWORDS.begin(), KEYWORDS.end(), str) != KEYWORDS.end();
}

bool Lexer::is_separator(std::string &str) {
    return std::find(SEPARATORS.begin(), SEPARATORS.end(), str) != SEPARATORS.end();
}

bool Lexer::is_number(std::string &str) {
    for (char &c : str)
        if (!(c >= '0' && c <= '9'))
            return false;
    return true;
}

bool Lexer::is_word(std::string &str) {
    for (char &c : str)
        if (!(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') && c != '_' && !(c >= '0' && c <= '9'))
            return false;
    return true;
}

bool Lexer::is_string(std::string &str) {
    size_t n = str.length();
    return (n >= 2 && str[0] == '"' && str[n - 1] == '"');
}

bool Lexer::is_newline(std::string &str) {
    size_t n = str.length();
    return (n == 1 && str[0] == '\n') || (n == 2 && str[0] == '\r' && str[1] == '\n');
}

Lexer::Lexer(Scanner &scanner) : m_scanner(scanner) {}

Token Lexer::get_token() {
    // Read characters until a new Token is produced
    while (true) {
        int c = m_scanner.get_char();

        // End of file
        if (c == -1) {
            if (m_sstream_length == 0)
                return {END_OF_FILE, std::string(), m_line, m_position};
            if (!tokenize(m_sstream.str()))
                throw LexerException(m_line, m_position, "unexpected end of file");
            return make_token();
        }

        // Whitespace: always marks the end of a get_token (if there currently is one)
        if (c == ' ' || c == '\t') {
            if (m_sstream_length == 0)
                continue;
            return make_token();
        }

        // Line comments (--): mark the end of a get_token (if there currently is one),
        // then continue discarding characters until a newline appears
        if (c == '-' && m_scanner.get_previous_char() == '-') {
            m_sstream.seekp(-1, m_sstream.cur);
            m_sstream_length--; // Remove the first '-'

            // TODO: maybe this is unnecessary ?? after the first '-' one would already have made a get_token
            Token token;
            bool token_before = (m_sstream_length > 0);
            if (token_before)
                token = make_token();

            do {
                c = m_scanner.get_char();
            } while (c != -1 && c != '\n');

            if (c == '\n') { // do include the newlines themselves
                m_sstream << (char) c;
                m_sstream_length++;
            }

            m_line = m_scanner.get_line();
            m_position = m_scanner.get_position();
            tokenize(m_sstream.str());

            return token_before ? token : get_token();
        }

        // Block comments (/- * -/): do a similar thing as line comments
        if (c == '-' && m_scanner.get_previous_char() == '/') {
            // Remove the '/' from the sb before making a get_token!
            m_sstream.seekp(-1, m_sstream.cur);
            m_sstream_length--; // Remove the first '/'

            Token token;
            bool token_before = (m_sstream_length > 0);
            if (token_before)
                token = make_token();

            do {
                c = m_scanner.get_char();
            } while (c != -1 && !(m_scanner.get_previous_char() == '-' && c == '/'));

            m_line = m_scanner.get_line();
            m_position = m_scanner.get_position();
            tokenize(m_sstream.str());

            return token_before ? token : get_token();
        }

        // If string buffer is empty, set line and column of next get_token
        if (m_sstream_length == 0) {
            m_line = m_scanner.get_line();
            m_position = m_scanner.get_position();
        }

        // Enlarge the get_token if possible
        m_sstream << (char) c;
        m_sstream_length++;

        // If can tokenize, just continue
        if (tokenize(m_sstream.str()))
            continue;

        // If we also did not tokenize before, hope that it will make sense later
        if (m_current_token.m_type == NONE)
            continue;

        // Return the last valid get_token
        Token token = make_token();
        m_sstream << (char) c;
        m_sstream_length++;

        m_line = m_scanner.get_line();
        m_position = m_scanner.get_position();
        tokenize(m_sstream.str());

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
        std::string message = "unknown get_token '" + m_sstream.str() + "'";
        m_sstream.str(std::string());
        m_sstream_length = 0;
        throw LexerException(m_line, m_position, message);
    }

    Token token = m_current_token;
    m_current_token.m_type = NONE;
    m_current_token.m_line = m_scanner.get_line(); // TODO: set these correctly at the correct place
    m_current_token.m_position = m_scanner.get_position();
    m_sstream.str(std::string());
    m_sstream_length = 0;
    return token;
}
