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

//Run tasks parallely, Multhithreading comes in place from here
//void runParallel(std::vector<Task>& tasks, const Dependents_Map& dependents, Indegree_Map indegree, ThreadPool& pool);
//function is defined before Thread Pool, causing syntax error

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

/*
    Class thread pool (Standard setup)

    Constructor: 
    It'll create threads
    Using condition variable 
    Will place tasks in a queue to follow upon with execution
    Will run until stop is called and queue becomes empty
    Makes sure all tasks are run before stopping

    Submit: 
    Adds task to the queue and notifies one of the thread to perform the task
    Uses rvalues for maximum efficiency

    Destructor:
    Notifies all to stop
    Joins threads to main thread


*/

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


/*
    Run parallel function to use multiple threads to perform tesks.
    We'll use a atomic variable "done" to check if all tasks are completed
    Created a hashmap with task id's and their address for O(1) lookup
    Assigning no. of pending tasks to indegree since the constructor in task.h initializes it with 0
    lambda funtion to send submit request to the thread pool
    recursively calling for when number of pending tasks hits 0
*/
void runParallel(std::vector<Task>& tasks, const Dependents_Map& dependents, Indegree_Map indegree, ThreadPool& pool){
    std::atomic <int> done{0};
    std::mutex lock_done;
    std::condition_variable cv_done;
    std::unordered_map<int, Task*> task_addr;

    for (auto& task : tasks) {
        task_addr[task.id] = &task;
    }

     for (auto& task : tasks) {
        task.pending = indegree[task.id];
    }

    std::function<void(const Task*)> submitTask = [&](const Task* task) {
        pool.submit([&, task]() {
            task->work();

            auto it = dependents.find(task->id);
            if(it != dependents.end()){
                for(auto& id : it->second){
                    int rem = --(task_addr[id]->pending);
                    if(rem == 0)
                        submitTask(task_addr[id]);
                }
            }
            int finished = ++done;
            if(finished == static_cast<int>(tasks.size())){
                cv_done.notify_all();
            }
        });
    };

    for(const auto& task : tasks){
        if(task.pending == 0){
            submitTask(&task);
        }
    }

    std::unique_lock<std::mutex> lk(lock_done);
    cv_done.wait(lk, [&]{ 
        return done.load() == static_cast<int>(tasks.size()); }
    ); 
}