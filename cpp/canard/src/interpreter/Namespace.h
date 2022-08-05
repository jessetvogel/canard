//
// Created by Jesse Vogel on 01/07/2021.
//

#pragma once

#include <unordered_map>
#include <string>
#include "../core/Context.h"

class Session;

class Namespace {
public:

    Namespace();
    Namespace(Namespace &, std::string);

    Context &get_context();
    FunctionRef get_function(const std::string &);
    const std::vector<FunctionRef> &get_functions() { return m_functions; }
    Namespace *get_parent();
    std::vector<Namespace *> get_children();
    Namespace *get_namespace(const std::string &);
    void put_function(const FunctionRef &);
    Namespace *create_subspace(const std::string &);

    std::string to_string();

private:

    const std::string m_name;
    Namespace *m_parent = nullptr;
    std::unordered_map<std::string, std::unique_ptr<Namespace>> m_children;
    Context m_context;
    std::vector<FunctionRef> m_functions; // TODO: should we use a vector here?

};
