////
//// Created by Jesse Vogel on 05/12/2021.
////
//
//#include "FunctionPool.h"
//#include "Formatter.h"
//#include <array>
//
//#define MAX_CHUNK_SIZE (4096)
//
//FunctionRef FunctionPool::create_Type() {
//    // Reserve a Function
//    Function *f = reserve();
//    // Initialize as Type
//    f->init();
//    // Return a FunctionRef
//    return {this, f};
//}
//
//FunctionRef FunctionPool::create_function(const FunctionRef &type, FunctionParameters parameters) {
//    // Reserve a Function
//    Function *f = reserve();
//    // Set Function values
//    f->init(type, std::move(parameters));
//    // Return a FunctionRef
//    return {this, f};
//}
//
//FunctionRef FunctionPool::create_specialization(const FunctionRef &type,
//                                                FunctionParameters parameters,
//                                                const FunctionRef &base,
//                                                std::vector<FunctionRef> arguments) {
//    // Reserve a Function
//    Function *f = reserve();
//    // Set Function values
//    f->init(type, std::move(parameters), base, std::move(arguments));
//    // Return a FunctionRef
//    return {this, f};
//}
//
//Function *FunctionPool::reserve() {
//    // Functions should be reserved one at a time
//    std::unique_lock<std::mutex> lock(m_mutex);
//
//    // Check for recyclable function
//    if (!m_recyclable.empty()) {
//        Function *f = m_recyclable.front();
//        m_recyclable.pop();
//        return f;
//    }
//
//    // Create a new chunk if needed
//    size_t index = (m_function_counter++) % MAX_CHUNK_SIZE;
//    if (index == 0) {
////        CANARD_LOG("Allocating block #" << (m_function_counter / MAX_CHUNK_SIZE));
//        m_chunks.push_back(new Function[MAX_CHUNK_SIZE]);
//    }
//
//    // Now add a new Function to the last chunk
//    Function *f = m_chunks.back() + index;
//    return f;
//}
//
//void FunctionPool::recycle(Function *f) {
//    m_mutex.lock();
//    m_recyclable.push(f);
//    m_mutex.unlock();
//}
//
//FunctionPool::~FunctionPool() {
//    for (auto chunk: m_chunks)
//        delete[] chunk;
//}
