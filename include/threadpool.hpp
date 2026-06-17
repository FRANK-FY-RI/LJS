#ifndef __THREADPOOL_HPP
#define __THREADPOOL_HPP

#include "threadsafe_queue.hpp"
#include "function_wrapper.hpp"
#include <future>
#include <vector>
#include <thread>
#include <functional>

class threadpool {
    threadsafe_queue<function_wrapper> q;
    std::vector<std::thread> threads;
    unsigned int n_threads;

    void thread_work() {
        function_wrapper task;
        while(q.wait_and_pop(task)) { 
            task();
        }
    }

public:

    threadpool(unsigned int n) : n_threads(n) {
        for(unsigned int i = 0; i<n_threads; i++) {
            threads.push_back(
                std::thread(&threadpool::thread_work, this)
            );
        }
    }
    
    template <typename func>
    auto submit(func&& f) {
        using result_type = std::invoke_result_t<func>;
        std::packaged_task<result_type()> task(std::move(f));
        auto fut = task.get_future();
        q.push(std::move(task));
        return fut;
    }

    ~threadpool() {
        q.close();
        for(auto &it:threads) it.join();
    }
};


#endif
