//
// Created by Jesse Vogel on 27/11/2021.
//

#include "ThreadManager.h"
#include "../core/macros.h"

ThreadManager::ThreadManager(int num_threads) : m_max_threads(num_threads) {}

bool ThreadManager::wait_for_update() {
    std::unique_lock<std::mutex> lock(m_mutex);
    ++m_waiting_threads;

    // If number of waiting threads now equals total number of threads, this means all threads are waiting,
    // so we send 'false' to all of them
    if (m_waiting_threads == m_max_threads) {
        send_update(false);
        return false;
    }

    // Otherwise, wait for another thread to send an update value
    // However, we only do this if there is not a permanent update
    if (!m_update_permanent)
        m_cv.wait(lock);

    // Do not forget to decrease number of waiting threads!
    --m_waiting_threads;
    return m_update_value;
}

void ThreadManager::send_update(bool update) {
    m_update_value = update;
    m_cv.notify_all();
}

void ThreadManager::send_permanent_update(bool update) {
    m_update_value = update;
    m_update_permanent = true;
    m_cv.notify_all();
}

void ThreadManager::join_all() {
    for (auto &thread: m_threads) {
        if (thread.joinable())
            thread.join();
    }
}
