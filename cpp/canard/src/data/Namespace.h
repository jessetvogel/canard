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

    Context &context() { return m_context; }
    const std::string &name() const { return m_name; }

    Namespace *parent() const;
    std::vector<Namespace *> children();

    Namespace *create_subspace(const std::string &); // TODO: as put() ?
    Namespace *get_namespace(const std::string &);

    std::string full_name() const;

    const FunctionRef &get_function(const std::string &);
    const FunctionRef &resolve_function(const std::string &);

private:

    const std::string m_name;
    Namespace *m_parent = nullptr;
    std::unordered_map<std::string, std::unique_ptr<Namespace>> m_children;
    Context m_context;

};
