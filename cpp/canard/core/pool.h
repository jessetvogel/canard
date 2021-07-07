//
// Created by Jesse Vogel on 06/07/2021.
//

#pragma once

#include <vector>
#include <queue>

template<class T>
class Pool {

    struct Block {

        Pool *m_pool;
        std::unique_ptr<T> m_object;
        int m_reference_count;

        Block(Pool *, std::unique_ptr<T>);

        void increase();

        void decrease();

    };

    std::vector<std::unique_ptr<Block>> m_blocks;
    std::queue<Block *> m_unused;

public:

    struct Ptr {

    private:

        Block *m_block;

    public:

        explicit Ptr(Block &block) {
            m_block = &block;
            m_block->increase();
        }

        Ptr(std::nullptr_t) {
            m_block = nullptr;
        }

        Ptr(const Ptr &other) {
            m_block = other.m_block;
            m_block->increase();
        }

        ~Ptr() {
            m_block->decrease();
        }

        inline T* operator->() const { return m_block->m_object.get(); }

    };

    template<typename ...Args>
    Ptr create(Args ... args);

    void add_unused(Block *);

};
