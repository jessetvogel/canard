//
// Created by Jesse Vogel on 01/07/2021.
//

#pragma once

#include <unordered_map>
#include <string>
#include "Context.h"

class Session;

class Namespace {
public:

    Namespace();
    Namespace(Namespace &, std::string);

    Namespace *parent() const { return m_parent; }
    const std::string &name() const { return m_name; }
    const std::string &full_name() const { return m_full_name; }
    Context &context() { return m_context; }
    const Context &context() const { return m_context; }
    const std::unordered_map<std::string, std::unique_ptr<Namespace>> &children() const { return m_children; }

    Namespace *create_subspace(const std::string &);
    Namespace *get_namespace(const std::string &);

    const FunctionRef &get_function(const std::string &);

private:

    const std::string m_name;
    const std::string m_full_name;
    Namespace *const m_parent = nullptr;
    std::unordered_map<std::string, std::unique_ptr<Namespace>> m_children;
    Context m_context;

};
