//
// Created by Jesse Vogel on 01/07/2021.
//

#pragma once

#include <unordered_map>
#include <string>
#include "context.h"

class Session;

class Namespace {

    const Session &m_session;
    Namespace *m_parent;
    std::unordered_map<std::string, std::unique_ptr<Namespace>> m_children;

    const std::string m_name;
    Context m_context;

public:

    explicit Namespace(Session &);

    explicit Namespace(Namespace&, std::string);

    Context &get_context();

    FunctionPtr get_function(std::string &);

    Namespace* get_parent();

    Namespace* get_namespace(std::string &);

    Namespace *create_subspace(const std::string& basicString);

    std::string to_string();

};
