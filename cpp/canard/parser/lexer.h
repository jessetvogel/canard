//
// Created by Jesse Vogel on 01/07/2021.
//

#pragma once

#include <utility>
#include <vector>
#include <sstream>
#include "scanner.h"

enum TokenType {
    NONE = 0,
    IDENTIFIER = 1,
    SEPARATOR = 2,
    KEYWORD = 3,
    NEWLINE = 4,
    NUMBER = 5,
    STRING = 6,
    END_OF_FILE = 7
};

struct Token {

    TokenType m_type;
    std::string m_data;
    int m_line;
    int m_position;

};

class Lexer {

    static const std::vector<std::string> KEYWORDS, SEPARATORS;

    static bool is_keyword(std::string &);

    static bool is_separator(std::string &);

    static bool is_number(std::string &);

    static bool is_word(std::string &);

    static bool is_string(std::string &);

    static bool is_newline(std::string &);

    Scanner &m_scanner;

    Token m_current_token = {NONE};
    std::stringstream m_sstream;
    int m_sstream_length = 0;
    int m_line = 0, m_position = 1;

    bool tokenize(std::string);

    Token make_token();

public:

    explicit Lexer(Scanner &);

    Token get_token();

};

struct LexerException : public std::exception {

    const int m_line, m_position;
    const std::string m_message;

    explicit LexerException(int line, int position, std::string message) : m_line(line), m_position(position),
                                                                           m_message(std::move(message)) {};

};
