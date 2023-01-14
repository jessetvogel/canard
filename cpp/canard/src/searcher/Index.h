//
// Created by Jesse Vogel on 26/08/2022.
//

#pragma once

#include "../data/Context.h"
#include <unordered_set>
#include <vector>

class Index {
public:

    explicit Index(const std::unordered_set<Context *> &);

    const std::vector<FunctionRef> &all_theorems() const { return m_all_theorems; }
    const std::vector<FunctionRef> &generic_theorems() const { return m_generic_theorems; }
    const std::vector<FunctionRef> *theorems(const FunctionRef &) const;
    
private:

    std::vector<FunctionRef> m_all_theorems, m_generic_theorems;
    std::unordered_map<FunctionRef, std::vector<FunctionRef>> m_index;

};
