//
// Created by Jesse Vogel on 01/07/2021.
//

#pragma once

#include <utility>
#include <vector>
#include <unordered_set>
#include <sstream>
#include <queue>
#include "Scanner.h"

enum TokenType {
    NONE = 0,
    IDENTIFIER = 1,
    SEPARATOR = 2,
    KEYWORD = 3,
    NEWLINE = 4,
    NUMBER = 5,
    STRING = 6,
    END_OF_FILE = 7,
    COMMENT = 8
};

struct Token {

    TokenType m_type;
    std::string m_data;
    int m_line;
    int m_position;

};

class Lexer {
public:

    static const char *to_string(const TokenType &);

    explicit Lexer(Scanner &);

    Token get_token();

private:
    static const std::unordered_set<std::string> &keywords();
    static const std::unordered_set<std::string> &separators();

    static bool is_keyword(const std::string &);
    static bool is_separator(const std::string &);
    static bool is_number(const std::string &);
    static bool is_word(const std::string &);
    static bool is_string(const std::string &);
    static bool is_newline(const std::string &);

    Scanner &m_scanner;

    Token m_current_token = {NONE};
    std::stringstream m_stream;
    int m_stream_length = 0;

    std::queue<Token> m_queue;

    bool tokenize(std::string);
    Token make_token();
    void reset_token();

    void stream_remove_back();
};

struct LexerException : public std::exception {

    const int m_line, m_position;
    const std::string m_message;

    explicit LexerException(int line, int position, std::string message) : m_line(line), m_position(position),
                                                                           m_message(std::move(message)) {};

};
