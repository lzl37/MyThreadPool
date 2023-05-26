#include <iostream>
#include "ThreadPool.h"
int main() {
    ThreadPool threadPool(10);

    auto fun = [](const std::string& str){
        std::cout << str << std::endl;
        return str.size();
    };

    std::vector<std::future<unsigned long>> results;
    results.reserve(10);
    for(int i = 0; i < 10; ++i){
        results.push_back(
                    threadPool.addTask(fun, std::string("123"))
                );
    }

    for(auto&& f : results){
        std::cout << f.get() << std::endl;
    }
    return 0;
}
