//
// Created by Jesse Vogel on 01/07/2021.
//

#pragma once

#include <unordered_map>
#include <unordered_set>
#include <string>
#include "Context.h"

#define DEFAULT_PREFERENCE (100)

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

    void get_all_subspaces(std::unordered_set<Namespace *> &set);

    Namespace &get_subspace(const std::string &);
    Namespace *find_subspace(const std::string &);

    const FunctionRef &get_function(const std::string &);

    void set_preference(const FunctionRef &, int);
    int get_preference(const FunctionRef &);

private:

    const std::string m_name;
    const std::string m_full_name;
    Namespace *const m_parent = nullptr;
    std::unordered_map<std::string, std::unique_ptr<Namespace>> m_children;
    Context m_context;
    std::unordered_map<FunctionRef, int> m_preferences;

    Namespace *create_subspace(const std::string &);

};
