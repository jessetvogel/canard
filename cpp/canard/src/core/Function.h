//
// Created by Jesse Vogel on 01/07/2021.
//

#pragma once

#include <utility>
#include <string>
#include <vector>
#include <unordered_set>
#include <atomic>

class Namespace;
class Function;
class FunctionPool;
struct FunctionParameters;

struct FunctionPtr {
    FunctionPtr() = default;
    FunctionPtr(std::nullptr_t) {};
    FunctionPtr(FunctionPool *, Function *);
    FunctionPtr(const FunctionPtr &other); // copy constructor
    ~FunctionPtr();

    const FunctionPtr &type() const;
    const FunctionPtr &base() const;
    FunctionPool &pool() const { return *m_pool; }

    FunctionPtr specialize(std::vector<FunctionPtr>, FunctionParameters);

    inline bool operator==(std::nullptr_t) const { return m_f == nullptr; }
    inline bool operator!=(std::nullptr_t) const { return m_f != nullptr; }
    inline bool operator==(const FunctionPtr &other) const { return m_f == other.m_f; }
    inline bool operator!=(const FunctionPtr &other) const { return m_f != other.m_f; }
    inline Function *operator->() const { return m_f; }
    FunctionPtr &operator=(FunctionPtr other);

    bool equivalent(const FunctionPtr &) const;

private:
    FunctionPool *m_pool = nullptr;
    Function *m_f = nullptr;
};

struct FunctionParameters {
    std::vector<FunctionPtr> m_functions;
    std::vector<bool> m_explicits;

    inline size_t size() const { return m_functions.size(); }
    inline size_t empty() const { return m_functions.empty(); }
};

struct Function {
public:

    const FunctionParameters &parameters() const;
    const std::vector<FunctionPtr> &arguments() const;
    std::vector<FunctionPtr> explicit_parameters() const;

    const std::string &label() const;
    Namespace *space() const;
    void set_label(const std::string &label);
    void set_namespace(Namespace *space);

    inline bool is_base() const { return m_base == nullptr; }
    bool depends_on(const std::vector<FunctionPtr> &);
    bool depends_on(const std::unordered_set<FunctionPtr> &);
    bool signature_depends_on(const std::vector<FunctionPtr> &);

private:

    Namespace *m_space;
    std::string m_label;

    FunctionPtr m_type = nullptr;
    FunctionPtr m_base = nullptr;
    FunctionParameters m_parameters;
    std::vector<FunctionPtr> m_arguments;

    std::atomic<uint32_t> m_reference_count;

    void init();
    void init(const FunctionPtr &type, FunctionParameters parameters);
    void init(const FunctionPtr &type, FunctionParameters parameters,
              const FunctionPtr &base, std::vector<FunctionPtr> arguments);

    friend class FunctionPool;
    friend class FunctionPtr;

};

struct SpecializationException : public std::exception {

    const std::string m_message;

    explicit SpecializationException(std::string message) : m_message(std::move(message)) {};

};

#include <utility>

template<>
struct std::hash<FunctionPtr> {
    std::size_t operator()(const FunctionPtr &ptr) const {
        return reinterpret_cast<std::size_t>(ptr.operator->());
    }
};
