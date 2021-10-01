//
// Created by Jesse Vogel on 01/07/2021.
//

#ifndef CANARD_SCANNER_H
#define CANARD_SCANNER_H

#include <istream>
#include <iostream>

class Scanner {

    std::istream& m_istream;

    int m_line = 1, m_column = 0;
    int m_current_char = -1, m_previous_char = -1;

public:

    explicit Scanner(std::istream& istream) : m_istream(istream) {};

    int get_char();
    int get_previous_char() const;

    int line() const;
    int position() const;

};


#endif //CANARD_SCANNER_H
