//
// Created by Jesse Vogel on 01/07/2021.
//

#pragma once

#include <utility>
#include <string>
#include <vector>

class Function;

class Namespace;

typedef std::shared_ptr<Function> FunctionPtr;

#include "namespace.h"

struct DependencyData {
    std::vector<FunctionPtr> m_functions;
    std::vector<bool> m_explicits;

    inline size_t size() const { return m_functions.size(); }
    inline size_t empty() const { return m_functions.empty(); }

};

class Function : public std::enable_shared_from_this<Function> {

protected:

    Namespace *m_space = nullptr;
    std::string m_label;

    FunctionPtr m_type;
    DependencyData m_dependencies;

public:

    Function(FunctionPtr, DependencyData);

    FunctionPtr get_type();

    const DependencyData &get_dependencies() { return m_dependencies; }

    std::vector<FunctionPtr> get_explicit_dependencies();

    const std::string &get_label() { return m_label; }

    void set_label(const std::string &label);

    void set_namespace(Namespace *space);

    virtual FunctionPtr get_base();

    virtual const std::vector<FunctionPtr> &get_arguments();

    virtual bool depends_on(const std::vector<FunctionPtr> &);

    virtual bool depends_on(const std::unordered_set<FunctionPtr> &);

    bool signature_depends_on(const std::vector<FunctionPtr> &);

    std::string to_string();

    virtual std::string to_string(bool, bool);

    FunctionPtr specialize(std::vector<FunctionPtr>, DependencyData);

    bool equals(const FunctionPtr &);

};

struct SpecializationException : public std::exception {

    std::string m_message;

    SpecializationException(std::string message) : m_message(std::move(message)) {};

};
