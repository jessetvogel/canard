//
// Created by Jesse Vogel on 06/07/2021.
//

#include "pool.h"
#include "function.h"
#include "specialization.h"

template<class T>
Pool<T>::Block::Block(Pool *pool, std::unique_ptr<T> object) {
    m_pool = pool;
    m_object = std::move(object);
    m_reference_count = 0;
}

template<class T>
void Pool<T>::Block::increase() {
    ++m_reference_count;
}

template<class T>
void Pool<T>::Block::decrease() {
    if (--m_reference_count == 0)
        m_pool->add_unused(this);
}

template<class T>
template<typename... Args>
typename Pool<T>::Ptr Pool<T>::create(Args... args) {
    // If there is an unused object, use that one
    if (!m_unused.empty()) {
        Block *block = m_unused.front();
        m_unused.pop();
        *block->m_object = std::move(T(std::forward<Args>(args)...));
        return Ptr(*block);
    }

    // Otherwise, create a new one, and store the control block in the vector
    auto block = std::make_unique<Block>(this, std::make_unique<T>(std::forward<Args>(args)...));
    Ptr ptr(*block);
    m_blocks.push_back(std::move(block));
    return ptr;
}

template<class T>
void Pool<T>::add_unused(Pool::Block *block) {
    m_unused.push(block);
}

// Explicit instantiations
template class Pool<Function>;
template class Pool<Specialization>;
template Pool<Function>::Ptr Pool<Function>::create<>(FunctionPtr, DependencyData);
template Pool<Specialization>::Ptr Pool<Specialization>::create<>(FunctionPtr, std::vector<FunctionPtr>, FunctionPtr, DependencyData);
