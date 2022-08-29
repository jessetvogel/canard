//
// Created by Jesse Vogel on 01/07/2021.
//

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "Telescope.h"

class Function;

class FunctionRef {
public:

    static const FunctionRef &null();

    FunctionRef() = default;
    FunctionRef(std::nullptr_t) {};
    FunctionRef(std::shared_ptr<Function>);
    FunctionRef(const FunctionRef &other); // copy constructor

    const FunctionRef &type() const;
    const FunctionRef &base() const;

    FunctionRef specialize(const Telescope &parameters, std::vector<FunctionRef> arguments) const;

    bool equivalent(const FunctionRef &) const;

    bool depends_on(const std::vector<FunctionRef> &) const;
    bool signature_depends_on(const std::vector<FunctionRef> &) const;

    inline explicit operator bool() const { return m_f != nullptr; }
    inline bool operator==(std::nullptr_t) const { return m_f == nullptr; }
    inline bool operator!=(std::nullptr_t) const { return m_f != nullptr; }
    inline bool operator==(const FunctionRef &other) const { return m_f == other.m_f; }
    inline bool operator!=(const FunctionRef &other) const { return m_f != other.m_f; }
    inline Function *operator->() const { return m_f.get(); }
    FunctionRef &operator=(FunctionRef other);

protected:

    std::shared_ptr<Function> m_f = nullptr;
};

class Function {
public:

    static FunctionRef make(Telescope parameters, const FunctionRef &type);
    static FunctionRef make(Telescope parameters, const FunctionRef &type, const FunctionRef &base, std::vector<FunctionRef> arguments);

    inline const Telescope &parameters() const { return m_parameters; }
    const std::vector<FunctionRef> &arguments() const;
    inline bool is_base() const { return m_base == nullptr; }
    bool is_constructor() const;

    const std::string &name() const { return m_name; };
    void set_name(const std::string &);
    inline bool implicit() const { return m_implicit; }
    void set_implicit(bool);
    inline const FunctionRef &constructor() const { return m_constructor; }
    void set_constructor(const FunctionRef &);
    inline void *space() const { return m_space; }
    void set_space(void *);

protected:

    Function(Telescope parameters, const FunctionRef &type);
    Function(Telescope parameters, const FunctionRef &type, const FunctionRef &base, std::vector<FunctionRef> arguments);

private:

    std::string m_name;
    FunctionRef m_type = nullptr;
    FunctionRef m_base = nullptr;
    Telescope m_parameters;
    std::vector<FunctionRef> m_arguments;

    bool m_implicit = false;
    FunctionRef m_constructor = nullptr;
    void *m_space = nullptr;

    friend class FunctionRef;
};

struct SpecializationException : public std::exception {

    const std::string m_message;
    const std::unordered_map<std::string, FunctionRef> m_map;

    SpecializationException(std::string message, std::unordered_map<std::string, FunctionRef> map)
            : m_message(std::move(message)), m_map(std::move(map)) {};
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
