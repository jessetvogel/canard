//
// Created by Jesse Vogel on 01/07/2021.
//

#pragma once

#include <unordered_map>
#include <string>
#include "Context.h"

class Session;

class Namespace {

    const Session &m_session;
    Namespace *m_parent;
    std::unordered_map<std::string, std::unique_ptr<Namespace>> m_children;

    const std::string m_name;
    Context m_context;

    std::vector<FunctionPtr> m_all_functions;

public:

    explicit Namespace(Session &);

    explicit Namespace(Namespace&, const std::string&);

    Context &get_context();

    FunctionPtr get_function(const std::string &);

    void put_function(const FunctionPtr&);

    const std::vector<FunctionPtr>& get_functions() { return m_all_functions; }

    Namespace* get_parent();

    Namespace* get_namespace(const std::string &);

    Namespace *create_subspace(const std::string&);

    std::string to_string();

};
