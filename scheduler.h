#pragma once

#include <vector>
#include <queue>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include "task.h"
#include "graph.h"

//Find topological sorting of the graph given it is a DAG using Khan's Algorithm
std::vector<int> topologicalSort(const std::vector<Task>& tasks, const Dependents_Map& dependents, Indegree_Map indegree);

//Run tasks sequentially in the order of topological sort
void runSequential(const std::vector<Task>& tasks, const std::vector<int>& order);

std::vector<int> topologicalSort(const std::vector<Task>& tasks, const Dependents_Map& dependents, Indegree_Map indegree){
    std::queue<int> q;
    std::vector<int> order;

    for(const auto& task : tasks){
        if(indegree[task.id] == 0){
            q.push(task.id);
        }
    }

    while(!q.empty()){
        int id = q.front();
        q.pop();
        auto it = dependents.find(id);
        if(it != dependents.end()){
            for(int v : it->second){
                indegree[v]--;
                if(indegree[v] == 0)
                    q.push(v);
            }
        }
        order.push_back(id);
    }
    return order;
}

void runSequential(const std::vector<Task>& tasks, const std::vector<int>& order){
    std::unordered_map<int, const Task*> task_addr;
    for (const auto& task : tasks) {
        task_addr[task.id] = &task;
    }
    
    for(int id : order){
        auto it = task_addr.find(id);
        if(it != task_addr.end())
            it->second->work();
    }
}

class ThreadPool{
    private: 
        std::vector <std::thread> workers;
        std::queue <std::function<void()>> tasks;
        std::mutex q_mtx;
        std::condition_variable cv;
        std::atomic <bool> stop {false};
    
    public:
        ThreadPool(size_t threads){
            for(size_t i = 0; i < threads; i++){
                workers.emplace_back([this](){
                    while(true){
                        std::function<void()> task;{
                            std::unique_lock <std::mutex> lock(this->q_mtx);
                            this->cv.wait(lock, [this]{
                                return this->stop || !this->tasks.empty();
                            });

                            if(this->stop && this->tasks.empty())
                                return;
                            
                            task = std::move(this->tasks.front());
                            this->tasks.pop();
                        }
                        task();
                    }
                });
            }
        }
        
        template <typename F>
        void submit(F&& task){
            std::unique_lock <std::mutex> lock(q_mtx);
            if(stop)
                return;
            tasks.emplace(std::forward<F> (task));
            lock.unlock();
            cv.notify_one();
        }

        ~ThreadPool(){
            {
                std::unique_lock <std::mutex> lock(q_mtx);
                stop = true;
            }
            cv.notify_all();
            for(std::thread& worker : workers)
                if(worker.joinable())
                    worker.join();
        }
};