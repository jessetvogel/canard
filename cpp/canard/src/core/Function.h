//
// Created by Jesse Vogel on 01/07/2021.
//

#pragma once

#include <string>
#include <vector>
#include <unordered_set>
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

    inline explicit operator bool() const { return m_f != nullptr; }
    inline bool operator==(std::nullptr_t) const { return m_f == nullptr; }
    inline bool operator!=(std::nullptr_t) const { return m_f != nullptr; }
    inline bool operator==(const FunctionRef &other) const { return m_f == other.m_f; }
    inline bool operator!=(const FunctionRef &other) const { return m_f != other.m_f; }
    inline Function *operator->() const { return m_f.get(); }
    FunctionRef &operator=(FunctionRef other);
    inline std::shared_ptr<Function> get() const { return m_f; }

protected:

    std::shared_ptr<Function> m_f = nullptr;
};

class Function {
public:

    static FunctionRef make(Telescope parameters, const FunctionRef &type);
    static FunctionRef make(Telescope parameters, const FunctionRef &type, const FunctionRef &base, std::vector<FunctionRef> arguments);

    const Telescope &parameters() const;
    const std::vector<FunctionRef> &arguments() const;

    const std::string &name() const;
    void set_name(const std::string &);

    inline bool is_base() const { return m_base == nullptr; }
    bool depends_on(const std::vector<FunctionRef> &);
    bool depends_on(const std::unordered_set<FunctionRef> &);
    bool signature_depends_on(const std::vector<FunctionRef> &);

    void *metadata() const { return m_metadata.get(); }
    void set_metadata(std::shared_ptr<void>);

protected:

    Function(Telescope parameters, const FunctionRef &type);
    Function(Telescope parameters, const FunctionRef &type, const FunctionRef &base, std::vector<FunctionRef> arguments);

private:

    std::string m_name;
    FunctionRef m_type = nullptr;
    FunctionRef m_base = nullptr;
    Telescope m_parameters;
    std::vector<FunctionRef> m_arguments;

    std::shared_ptr<void> m_metadata = nullptr;

    friend class FunctionRef;
};

struct SpecializationException : public std::exception {

    const FunctionRef m_f, m_g;
    const std::string m_message;

    // TODO: this must really be done differently!
    SpecializationException(const FunctionRef &f, const FunctionRef &g, std::string message)
            : m_f(f), m_g(g), m_message(std::move(message)) {};
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
