#ifndef __FUNCTION_WRAPPER
#define __FUNCTION_WRAPPER

#include <memory>

class function_wrapper {

    struct func_base {
        virtual void call()=0;
        virtual ~func_base() {}
    };

    template <typename func>
    struct func_impl : func_base {
        func f; 
        func_impl(func&& f_) : f(std::move(f_)) {}
        void call() override { f(); }
    };
    
    std::unique_ptr<func_base> ptr;

public:

    template <typename func>
    function_wrapper(func&& f) :
        ptr(
            std::make_unique<func_impl<func>>(std::move(f))
        )
    {}

    function_wrapper()=default;

    function_wrapper(function_wrapper&)=delete;
    function_wrapper(const function_wrapper&)=delete;
    function_wrapper& operator=(const function_wrapper&)=delete;
    
    function_wrapper(function_wrapper&& other) noexcept :
        ptr(std::move(other.ptr))
    {}

    function_wrapper& operator=(function_wrapper&& other) noexcept {
        if(this == &other) return *this;
        ptr = std::move(other.ptr);
        return *this;
    }

    void operator()() {
        ptr->call();
    }
};


#endif