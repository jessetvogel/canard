//
// Created by Jesse Vogel on 01/07/2021.
//

#pragma once

#include "function.h"

class Specialization : public Function {

    FunctionPtr m_base;
    std::vector<FunctionPtr> m_arguments;

public:

    Specialization(const FunctionPtr& base, std::vector<FunctionPtr> arguments, const FunctionPtr& type, Function::Dependencies dependencies);

    FunctionPtr get_base() override;
    const std::vector<FunctionPtr>& get_arguments() override;
    bool depends_on(const std::vector<FunctionPtr>&) override;
    bool depends_on(const std::unordered_set<FunctionPtr>&) override;
    std::string to_string(bool, bool) override;

};
