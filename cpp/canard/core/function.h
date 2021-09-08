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

class Function : public std::enable_shared_from_this<Function> {

public:

    const int m_id;

    struct Dependencies {
        std::vector<FunctionPtr> m_functions;
        std::vector<bool> m_explicits;

        size_t size() const { return m_functions.size(); }
        size_t empty() const { return m_functions.empty(); }
    };

protected:

    Namespace *m_space = nullptr;
    std::string m_label;

    const FunctionPtr m_type;
    const Dependencies m_dependencies;

public:

    Function(FunctionPtr, Dependencies);

    FunctionPtr type();

    const std::string &label() { return m_label; }

    virtual FunctionPtr base() { return shared_from_this(); }

    const Dependencies &dependencies() { return m_dependencies; }

    virtual const std::vector<FunctionPtr> &arguments() { return m_dependencies.m_functions; }

    std::vector<FunctionPtr> explicit_dependencies();

    void set_label(const std::string &label) { m_label = label; }

    void set_namespace(Namespace *space) { m_space = space; }

    virtual bool depends_on(const std::vector<FunctionPtr> &);
    virtual bool depends_on(const std::unordered_set<FunctionPtr> &);
    bool signature_depends_on(const std::vector<FunctionPtr> &);
    std::string to_string();
    virtual std::string to_string(bool, bool);
    FunctionPtr specialize(std::vector<FunctionPtr>, Dependencies);
    bool equals(const FunctionPtr &);

};

struct SpecializationException : public std::exception {

    const std::string m_message;

    SpecializationException(std::string message) : m_message(std::move(message)) {};

};
