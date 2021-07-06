//
// Created by Jesse Vogel on 01/07/2021.
//

#pragma once

#include "function.h"

class Specialization : public Function {

    const FunctionPtr m_base;
    const std::vector<FunctionPtr> m_arguments;

public:

    Specialization(FunctionPtr base, std::vector<FunctionPtr> arguments, FunctionPtr type, DependencyData dependencies);

    FunctionPtr get_base() override;
    const std::vector<FunctionPtr>& get_arguments() override;
    bool depends_on(const std::vector<FunctionPtr>&) override;
    bool depends_on(const std::unordered_set<FunctionPtr>&) override;
    std::string to_string(bool, bool) override;

};
