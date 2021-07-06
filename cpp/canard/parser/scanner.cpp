//
// Created by Jesse Vogel on 01/07/2021.
//

#include "scanner.h"

int Scanner::get_char() {
    char c;
    if (!m_istream.get(c))
        return -1;

    if (c == '\n') {
        m_line++;
        m_column = 0;
    } else {
        m_column++;
    }

    m_previous_char = m_current_char;
    m_current_char = (unsigned char) c;

    return c;
}

int Scanner::get_previous_char() const {
    return m_previous_char;
}

int Scanner::get_line() const {
    return m_line;
}

int Scanner::get_position() const {
    return m_column;
}
