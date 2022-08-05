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

size_t Telescope::size() const {
    return m_functions.size();
}

bool Telescope::empty() const {
    return m_functions.empty();
}
Telescope Telescope::operator+(const Telescope &other) const {
    std::vector<FunctionRef> new_functions = m_functions;
    new_functions.insert(new_functions.end(), other.m_functions.begin(), other.m_functions.end());
    return Telescope(std::move(new_functions));
}

