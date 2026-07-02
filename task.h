#pragma once

#include <vector>
#include <functional>
#include <atomic>

/* Task is defined using its ID, the functions it works on
    and all other tasks it is depends upon(the prerequisites)*/
    
struct Task{
    int id;
    std::function <void()> work;
    std::vector <int> dependencies;
    std::atomic <int> pending {0};
    bool failed = false;

    Task() : id(0), work(nullptr), failed(false){}
    
    Task(int id_cp, const std::vector<int>& deps, std::function <void()> work_cp = nullptr)
     : id(id_cp), work(work_cp), dependencies(deps), pending(static_cast <int> (deps.size())), failed(false) {}

    Task(Task&& other) noexcept 
     :  id(other.id),  work(std::move(other.work)), dependencies(std::move(other.dependencies)), 
        pending(other.pending.load()), failed(other.failed) {}

    Task& operator= (Task&& other) noexcept{
        id = other.id;
        work = std::move(other.work);
        dependencies = std::move(other.dependencies);
        pending.store(other.pending.load());
        failed = other.failed;
        return *this;
    }   
};