#pragma once

#include <unordered_map>
#include <vector>
#include "task.h"

//Map to create adjacency list (using simpler names for cleaner code)
using Dependents_Map = std::unordered_map <int, std::vector <int>>;

// Map to store Indegree of each task (using simpler names for cleaner code)
using Indegree_Map = std::unordered_map <int, int>;

//Function definition to buid Directed Graph and store Indegree
void buildGraph(const std::vector<Task> & tasks, Dependents_Map& dependents, Indegree_Map& indegree);

//Function to check if given graph is a DAG
bool hasCycle(const Dependents_Map& dependents);

//enum so that both hasCycle and dfs can access it
enum visit{
        UNVISITED, 
        VISITING, 
        VISITED
};

void buildGraph(const std::vector<Task> & tasks, Dependents_Map& dependents, Indegree_Map& indegree){
    for(const auto& task: tasks){
        indegree[task.id] = static_cast<int>(task.dependencies.size());

        for(int dep: task.dependencies)
            dependents[dep].push_back(task.id);
    }
}

bool dfs(int u,const Dependents_Map& dependents, std::unordered_map<int, int>& state){
    state[u] = 1;
    auto it = dependents.find(u); 
    if(it != dependents.end()){      
        for(int v: it->second){
            if(state[v] == VISITING){
                return true;
            }
            if(state[v] == UNVISITED){
                if(dfs(v, dependents, state)){
                    return true;
                }
            }
        }
    }
    state[u] = VISITED;
    return false;
}

bool hasCycle(const Dependents_Map& dependents){
    std::unordered_map <int, int> state;
    for(auto& edge: dependents){
        int u = edge.first;
        if(state[u] == UNVISITED){
            if(dfs(u, dependents, state))
                return true;
        }
    }
    return false;
}
