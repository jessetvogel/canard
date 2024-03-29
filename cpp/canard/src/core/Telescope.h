//
// Created by Jesse Vogel on 03/08/2022.
//

#pragma once

#include <vector>
#include <cstddef>

class Function;

class FunctionRef;

class Telescope {
public:

    Telescope() = default;
    explicit Telescope(std::vector<FunctionRef>);

    void add(FunctionRef);
    void add(const std::vector<FunctionRef> &);
    void add(const Telescope &);

    bool contains(const FunctionRef &) const;

    inline const std::vector<FunctionRef> &functions() const { return m_functions; }
    size_t size() const { return m_functions.size(); }
    bool empty() const { return m_functions.empty(); }

    Telescope operator+(const Telescope &other) const;

    std::vector<Telescope> split() const;

private:

    std::vector<FunctionRef> m_functions;

};
