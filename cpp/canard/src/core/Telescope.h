//
// Created by Jesse Vogel on 03/08/2022.
//

#pragma once

#include <vector>
//#include "Function.h"

class Function;

struct FunctionRef;

class Telescope {
public:

    Telescope() = default;
    explicit Telescope(std::vector<FunctionRef>);

    void add(FunctionRef);

    inline const std::vector<FunctionRef> &functions() const { return m_functions; }
    size_t size() const;
    bool empty() const;

    Telescope operator+(const Telescope &other) const;

private:

    std::vector<FunctionRef> m_functions;

};
