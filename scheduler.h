#pragma once

#include <vector>
#include <queue>
#include <unordered_map>
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