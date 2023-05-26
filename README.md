# MyThreadPool
A simple and copied ThreadPool in C++

## 简易的线程池逻辑：



成员：

``` cpp
bool destructed;                          //标志着线程池是否被销毁  
std::queue<std::function<void()>> tasks;  //任务队列
std::vector<std::thread> workers;         //工作线程

std::mutex queueMutex;                    //用来保证对任务队列的读写安全
std::condition_variable cond;             //生产者消费者模型中用来通知工作线程
```



1. 构造时创建工作线程(std::vecotr<std::thread>)
每个工作线程一直死循环，每次循环从任务队列中取出一个任务执行，直到线程池销毁就执行完剩下的任务然后退出
``` cpp
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
// uniqueLock保证互斥地执行front和pop，防止在对队列写入的时候执行front和pop  
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
```


2. 添加任务逻辑：
由于每次传入的函数都可能不一样，所以使用通用的std::function<void()>来包装任务
使用参数包来传入可变参数，使用std::bind()来统一函数调用形式 (fun())，为了实现异步获取结果，我们将bind返回的可调用对象包装到std::packaged_task中，最后传入任务队列中


**并且注意std::packaged_task是 uncopyable的，只能通过std::shared_ptr来传入std::function**，



``` cpp
template<class Func, class... Arg>  
auto addTask(Func&& f, Arg&& ...args) ->std::future<decltype(f(args...))>{  
using return_type = decltype(f(args...));  
// 因为任务队列中存的是 void()类型的function，所以只能通过lambda将外部参数传入  
// 使用std::bind来统一函数调用方式，并且包装成异步任务  
auto task = std::make_shared< std::packaged_task<return_type()> >(  
std::bind(std::forward<Func>(f), std::forward<Arg>(args)...)  
);  
  
std::future<return_type> res = task->get_future();  
  
std::function<void()> func = [task](){  
(*task)();  
};  

std::unique_lock<std::mutex> lock(queueMutex);  
tasks.emplace(func);  
lock.unlock();  
  
cond.notify_one();  
  
return res;  
}
```

3. 析构函数
将标志位destructed置为true，同时唤醒所有线程（要么完成任务队列里的剩余任务，要么退出），最后让所有线程同步地执行完
``` cpp
~ThreadPool(){  
destructed = true;  
cond.notify_all();  
for(auto& thread : workers){  
thread.join();  
}  
}
```



