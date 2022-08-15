//
// Created by Jesse Vogel on 03/08/2022.
//

#include "Telescope.h"
#include "Function.h"

Telescope::Telescope(std::vector<FunctionRef> functions) {
    m_functions = std::move(functions);
}

void Telescope::add(FunctionRef f) {
    m_functions.push_back(std::move(f));
}

void Telescope::add(const std::vector<FunctionRef> &fs) {
    m_functions.insert(m_functions.end(), fs.begin(), fs.end());
}

void Telescope::add(const Telescope &other) {
    add(other.m_functions);
}

Telescope Telescope::operator+(const Telescope &other) const {
    std::vector<FunctionRef> new_functions = m_functions;
    new_functions.insert(new_functions.end(), other.m_functions.begin(), other.m_functions.end());
    return Telescope(std::move(new_functions));
}
