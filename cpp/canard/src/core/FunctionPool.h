////
//// Created by Jesse Vogel on 05/12/2021.
////
//
//#pragma once
//
//#include <queue>
//#include <mutex>
//#include "Function.h"
//
//class FunctionPool {
//public:
//
//    ~FunctionPool();
//
//    FunctionRef create_Type();
//    FunctionRef create_function(const FunctionRef &type, FunctionParameters parameters);
//    FunctionRef create_specialization(const FunctionRef &type, FunctionParameters parameters,
//                                      const FunctionRef &base, std::vector<FunctionRef> arguments);
//
//private:
//    std::mutex m_mutex;
//
//    size_t m_function_counter = 0;
//    std::queue<Function *> m_recyclable;
//    std::vector<Function *> m_chunks;
//
//    Function *reserve();
//    void recycle(Function *);
//
//    friend class FunctionPtr;
//
//};
