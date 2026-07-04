#pragma once

#include <vector>
#include <functional>
#include <atomic>

/* Task is defined using its ID, the functions it works on
    and all other tasks it is depends upon(the prerequisites)
    
    //Using an atomic variable, track number of pending tasks

    //Another atomic varaible to track Number of Retries 

    Using defualt constructor, copy constructor and move (and assignment) constructors to instantiate task objects safely
*/
    
struct Task{
    int id;
    std::function <bool()> work;
    std::vector <int> dependencies;
    std::atomic <int> pending {0};
    bool failed = false;
    int maxRetry = 0;
    std::atomic <int> Retry_count{0};

    Task() : id(0), work(nullptr), failed(false){}
    
    Task(int id_cp, 
        const std::vector<int>& deps, 
        std::function <bool()> work_cp = nullptr, 
        int maxRetries_cp = 0) :
        id(id_cp), work(work_cp), 
        dependencies(deps), 
        pending(static_cast <int> (deps.size())), 
        failed(false), 
        maxRetry(maxRetries_cp) {}

    Task(Task&& other) noexcept :
        id(other.id),  
        work(std::move(other.work)), 
        dependencies(std::move(other.dependencies)), 
        pending(other.pending.load()), 
        failed(other.failed) , 
        maxRetry(other.maxRetry), 
        Retry_count(other.Retry_count.load()) {}

    Task& operator= (Task&& other) noexcept{
        id = other.id;
        work = std::move(other.work);
        dependencies = std::move(other.dependencies);
        pending.store(other.pending.load());
        failed = other.failed;
        maxRetry = other.maxRetry;
        Retry_count.store(other.Retry_count.load());
        return *this;
    }   
};