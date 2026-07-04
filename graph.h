#pragma once

#include <unordered_map>
#include <vector>
#include "task.h"

// Map to create adjacency list (using simpler names for cleaner code)
using Dependents_Map = std::unordered_map <int, std::vector <int>>;

// Map to store Indegree of each task (using simpler names for cleaner code)
using Indegree_Map = std::unordered_map <int, int>;

// Function definition to buid Directed Graph and store Indegree
void buildGraph(const std::vector<Task> & tasks, Dependents_Map& dependents, Indegree_Map& indegree);

// Function to check if given graph is a DAG
bool hasCycle(const Dependents_Map& dependents);

// Function to run dfs to find cycles
bool dfs(int u,const Dependents_Map& dependents, std::unordered_map<int, int>& state);

// global enum so that both hasCycle and dfs can access it
enum visit{
        UNVISITED, 
        VISITING, 
        VISITED
};

void buildGraph(const std::vector<Task> & tasks, 
                Dependents_Map& dependents,
                Indegree_Map& indegree) {
    
    for(const auto& task: tasks){
        indegree[task.id] = static_cast<int>(task.dependencies.size()); // indegree is number of dependencies f the task

        for(int dep: task.dependencies)
            dependents[dep].push_back(task.id); // directed edge from dependency -> current_task
    }
}

bool dfs(int u,
         const Dependents_Map& dependents, 
         std::unordered_map<int, int>& state) {

    state[u] = 1;
    auto it = dependents.find(u); 
    if(it != dependents.end()){      
        for(int v: it->second){
            if(state[v] == VISITING){ // if vertex is in the current visiting group, we have a cycle
                return true;
            }
            if(state[v] == UNVISITED){ // dfs from its neighbour
                if(dfs(v, dependents, state)){
                    return true;
                }
            }
        }
    }
    state[u] = VISITED; // Mark node as visited once dfs is done
    return false;
}

bool hasCycle(const Dependents_Map& dependents){
    std::unordered_map <int, int> state;
    for(auto& edge: dependents){
        int u = edge.first; 
        if(state[u] == UNVISITED){ // dfs from all unvisited vertex
            if(dfs(u, dependents, state))
                return true;
        }
    }
    return false;
}
