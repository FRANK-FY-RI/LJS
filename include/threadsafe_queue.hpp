#ifndef __THREADSAFE_QUEUE
#define __THREADSAFE_QUEUE

#include <queue>
#include <condition_variable>
#include <memory>
#include <atomic>
#include <mutex>
#include <utility>

/*
lock based thread safe queue based on std::queue
*/

template <typename T>
class threadsafe_queue { 
    std::queue<T> q;
    mutable std::mutex mt;
    std::condition_variable cv;
    std::atomic<bool> closed{false};
    using guard_type = std::lock_guard<std::mutex>;
public:
    threadsafe_queue()=default;
    threadsafe_queue(const threadsafe_queue& other) {
        guard_type guard(other.mt);
        q = other.q;
    }
    threadsafe_queue& operator=(const threadsafe_queue&)=delete; 
    threadsafe_queue(threadsafe_queue&&)=delete;
    threadsafe_queue& operator=(threadsafe_queue&&)=delete;
    ~threadsafe_queue()=default;

    template <typename U>
    void push(U&& val) {
        guard_type guard(mt);
        q.push(std::forward<U>(val));
        cv.notify_one();
    }

    bool try_pop(T& res) {
        guard_type guard(mt);
        if(q.empty()) return false;
        res = std::move(q.front());
        q.pop();
        return true;
    }

    std::shared_ptr<T> try_pop() {
        guard_type guard(mt);
        if(q.empty()) return nullptr;
        std::shared_ptr<T> const res(
            std::make_shared<T>(std::move(q.front()))
        );
        q.pop();
        return res;
    }

    bool wait_and_pop(T& res) {
        std::unique_lock<std::mutex> uq(mt);
        cv.wait(uq, [this](){return closed || !q.empty();}); 
        if(closed) return false;
        res = std::move(q.front());
        q.pop();
        return true;
    }

    std::shared_ptr<T> wait_and_pop() {
        std::unique_lock<std::mutex> uq(mt);
        cv.wait(uq, [this](){return closed || !q.empty();});
        if(closed) return nullptr;
        std::shared_ptr<T> const res(
            std::make_shared<T>(std::move(q.front()))
        );
        q.pop();
        return res;
    }

    bool empty() const {
        guard_type guard(mt);
        return q.empty();
    }

    void close() {
        closed = true;
        cv.notify_all();
    }

};


#endif
