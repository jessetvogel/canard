//
// Created by Jesse Vogel on 05/12/2021.
//

#pragma once

#include <queue>
#include <mutex>
#include "Function.h"

class FunctionPool {
public:

    ~FunctionPool();

    FunctionPtr create_Type();
    FunctionPtr create_function(const FunctionPtr &type, FunctionParameters parameters);
    FunctionPtr create_specialization(const FunctionPtr &type, FunctionParameters parameters,
                                      const FunctionPtr &base, std::vector<FunctionPtr> arguments);

private:
    std::mutex m_mutex;

    size_t m_function_counter = 0;
    std::queue<Function *> m_recyclable;
    std::vector<Function *> m_chunks;

    Function *reserve();
    void recycle(Function *);

    friend class FunctionPtr;

};
