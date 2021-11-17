//
// Created by Jesse Vogel on 01/07/2021.
//

#include "Specialization.h"

#include <utility>
#include <sstream>

Specialization::Specialization(FunctionPtr base, std::vector<FunctionPtr> arguments, FunctionPtr type,
                               Function::Dependencies dependencies) : Function(std::move(type),
                                                                               std::move(dependencies)),
                                                                      m_base(std::move(base)),
                                                                      m_arguments(std::move(arguments)) {
    // TODO: maybe some assertions here
}

FunctionPtr Specialization::base() {
    return m_base;
}

const std::vector<FunctionPtr> &Specialization::arguments() {
    return m_arguments;
}

bool Specialization::depends_on(const std::vector<FunctionPtr> &list) {
    for (auto &arg: m_arguments) {
        if (arg->depends_on(list))
            return true;
    }
    return false;
}

bool Specialization::depends_on(const std::unordered_set<FunctionPtr> &set) {
    for (auto &arg: m_arguments) {
        if (arg->depends_on(set))
            return true;
    }
    return false;
}

