//
// Created by Jesse Vogel on 27/11/2021.
//

#pragma once

#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

class ThreadManager {
public:

    explicit ThreadManager(int);

    template<typename F, typename T, class... Args>
    void start(F, T, Args &&...);

    bool wait_for_update();
    void send_update(bool);
    void send_permanent_update(bool);
    void join_all();

private:

    const int m_max_threads;

    bool m_update_value = false;
    bool m_update_permanent = false;
    int m_waiting_threads = 0;

    std::mutex m_mutex;
    std::condition_variable m_cv;

    std::vector<std::thread> m_threads;

};

template<typename F, typename T, class... Args>
void ThreadManager::start(F function, T t, Args &&... args) {
    m_threads.clear();

    // Only start threads when using more than one threads
    if (m_max_threads > 1) {
        for (int i = 0; i < m_max_threads; ++i)
            m_threads.emplace_back(function, t, std::forward<Args>(args)...);
    } else {
        // Otherwise, just run the function on the main thread
        (t->*function)(std::forward<Args>(args)...);
    }
}
