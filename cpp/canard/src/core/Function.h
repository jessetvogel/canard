//
// Created by Jesse Vogel on 01/07/2021.
//

#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include "Telescope.h"

class Function;

class FunctionRef;

class FunctionRef {
public:

    FunctionRef() = default;
    FunctionRef(std::nullptr_t) {};
    FunctionRef(std::shared_ptr<Function>);
    FunctionRef(const FunctionRef &other); // copy constructor

    const FunctionRef &type() const;
    const FunctionRef &base() const;

    FunctionRef specialize(Telescope parameters, std::vector<FunctionRef> arguments) const;

    bool equivalent(const FunctionRef &) const;

    inline bool operator==(std::nullptr_t) const { return m_f == nullptr; }
    inline bool operator!=(std::nullptr_t) const { return m_f != nullptr; }
    inline bool operator==(const FunctionRef &other) const { return m_f == other.m_f; }
    inline bool operator!=(const FunctionRef &other) const { return m_f != other.m_f; }
    inline Function *operator->() const { return m_f.get(); }
    FunctionRef &operator=(FunctionRef other);

private:

    std::shared_ptr<Function> m_f = nullptr;
};

class Function {
public:

    static FunctionRef make(const FunctionRef &type, Telescope parameters);
    static FunctionRef make(const FunctionRef &type, Telescope parameters, const FunctionRef &base, std::vector<FunctionRef> arguments);

    const Telescope &parameters() const;
    const std::vector<FunctionRef> &arguments() const;

    const std::string &name() const;
    void set_name(const std::string &label);

    inline bool is_base() const { return m_base == nullptr; }
    bool depends_on(const std::vector<FunctionRef> &);
    bool depends_on(const std::unordered_set<FunctionRef> &);
    bool signature_depends_on(const std::vector<FunctionRef> &);

    void *metadata() const { return m_metadata.get(); }
    void set_metadata(std::shared_ptr<void>);

private:

    Function(const FunctionRef &type, Telescope parameters);
    Function(const FunctionRef &type, Telescope parameters, const FunctionRef &base, std::vector<FunctionRef> arguments);

    std::string m_name;
    FunctionRef m_type = nullptr;
    FunctionRef m_base = nullptr;
    Telescope m_parameters;
    std::vector<FunctionRef> m_arguments;

    std::shared_ptr<void> m_metadata = nullptr;

    friend class FunctionRef;
};

struct SpecializationException : public std::exception {

    const std::string m_message;

    explicit SpecializationException(std::string message) : m_message(std::move(message)) {};
};

#include <utility>

namespace std {
    template<>
    struct hash<FunctionRef> {
        size_t operator()(const FunctionRef &ptr) const {
            return reinterpret_cast<size_t>(ptr.operator->());
        }
    };
}
