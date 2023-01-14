//
// Created by Jesse Vogel on 01/07/2021.
//

#pragma once

#include <unordered_map>
#include "../core/Function.h"

#define DEFAULT_PREFERENCE (100)

class Context {
public:

    Context();
    Context(Context &);
    Context(Context &, std::string);

    Context *parent() const { return m_parent; }
    const std::string &name() const { return m_name; }
    const std::string &full_name() const { return m_full_name; }
    const std::unordered_map<std::string, FunctionRef> &functions() const { return m_functions; }
    const std::unordered_map<std::string, std::unique_ptr<Context>> &subspaces() const { return m_subspaces; }

    bool put(const std::string &, const FunctionRef &, bool only_reference = false);
    const FunctionRef &get(const std::string &) const;

    bool is_space() const { return m_is_space; }
    Context *find_subspace(const std::string &);
    const Context *find_subspace(const std::string &) const;
    Context &get_subspace(const std::string &);

    void set_preference(const FunctionRef &, int);
    int get_preference(const FunctionRef &);

private:

    Context *const m_parent = nullptr;
    const bool m_is_space;
    const std::string m_name;
    const std::string m_full_name;

    std::unordered_map<std::string, FunctionRef> m_functions;
    std::unordered_map<std::string, std::unique_ptr<Context>> m_subspaces;
    std::unordered_map<FunctionRef, int> m_preferences;

    Context *create_subspace(const std::string &);

};
