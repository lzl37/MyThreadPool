//
// Created by kingdog on 2023/5/25.
//

#ifndef MYTHREADPOOL_THREADPOOL_H

#define MYTHREADPOOL_THREADPOOL_H

#include <queue>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>

class ThreadPool{
private:
    bool destructed;
    std::queue<std::function<void()>> tasks;
    std::vector<std::thread> workers;

    std::mutex queueMutex;
    std::condition_variable cond;
public:
    explicit ThreadPool(size_t size) : destructed(false)
    {
        workers.reserve(size);

        auto perform = [&](){

            std::function<void()> task;

            while(true){
                std::unique_lock<std::mutex> uniqueLock(queueMutex);
                cond.wait(uniqueLock, [this](){return destructed || !tasks.empty();});
                if(destructed && tasks.empty()){
                    return ;
                }
                //  uniqueLock保证互斥地执行front和pop，防止在对队列写入的时候执行front和pop
                task = std::move(tasks.front());
                tasks.pop();

                uniqueLock.unlock();
                task();
            }

        };

        for(size_t i = 0; i < size; ++i){
            workers.emplace_back(perform);
        }
    }

    ~ThreadPool(){
        destructed = true;
        cond.notify_all();
        for(auto& thread : workers){
            thread.join();
        }
    }


    template<class Func, class... Arg>
    auto addTask(Func&& f, Arg&& ...args) ->std::future<decltype(f(args...))>{
        //  decltype也要使用万能引用
        using return_type = decltype( std::forward<Func>(f)(std::forward<Arg>(args)...) );
        //  因为任务队列中存的是 void()类型的function，所以只能通过lambda将外部参数传入
        //  使用std::bind来统一函数调用方式，并且包装成异步任务
        auto task = std::make_shared< std::packaged_task<return_type()> >(
                std::bind(std::forward<Func>(f), std::forward<Arg>(args)...)
                );
        //auto task = std::packaged_task<return_type()> ( std::bind(std::forward<Func>(f), std::forward<Arg>(args)...) );
        std::future<return_type> res = task->get_future();
        //std::future<return_type> res = task.get_future();

        std::function<void()> func = [task]
                () mutable ->void{
           // (*task)();
            (*task)();
        };

        std::unique_lock<std::mutex> lock(queueMutex);
        tasks.emplace(func);
        lock.unlock();
        cond.notify_one();

        return res;
    }


};



#endif
