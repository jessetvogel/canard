//
// Created by Jesse Vogel on 01/07/2021.
//

#pragma once

#include "Function.h"

class Specialization : public Function {

    FunctionPtr m_base;
    std::vector<FunctionPtr> m_arguments;

public:

    Specialization(FunctionPtr base, std::vector<FunctionPtr> arguments, FunctionPtr type,
                   Function::Dependencies dependencies);

    FunctionPtr base() override;
    const std::vector<FunctionPtr> &arguments() override;
    bool depends_on(const std::vector<FunctionPtr> &) override;
    bool depends_on(const std::unordered_set<FunctionPtr> &) override;

};
