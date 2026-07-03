#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <mutex>
#include <thread>
#include "task.h"
#include "graph.h"
#include "scheduler.h"

std::mutex logMtx;

auto makeWork(int id, bool shouldFail){
    return [id, shouldFail]() {
        {
            std::lock_guard<std::mutex> lk(logMtx);
            std::cout << "  -> task " << id << " running" << std::endl;
        }
        return !shouldFail;
    };
}

//Parses input file which contains details of the graph
std::vector<Task> loadInput(const std::string& filename){
    std::vector<Task> tasks;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Could not open file: " << filename << std::endl;
        return tasks;
    }

    std::string line;
    while (std::getline(file, line)){
        if (line.empty()) 
            continue; 

        std::stringstream ss(line);
        int id;
        if (!(ss >> id)) 
            continue;

        std::string flag = "P";
        std::string nextToken;
        
        if (ss >> nextToken) {
            if (nextToken == "F") {
                flag = "F";
            } 
            else if (nextToken == "P") {
                flag = "P";
            } 
            else {
                ss.seekg(-static_cast<std::streamoff>(nextToken.size()), std::ios_base::cur);
            }
        }

        bool shouldFail = (flag == "F");

        std::vector<int> dependencies;
        int dep_id;
        while (ss >> dep_id) {
            dependencies.push_back(dep_id);
        }

        tasks.emplace_back(id, dependencies, makeWork(id, shouldFail));
    }

    return tasks;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <graph_input_file>" << std::endl;
        return 1;
    }
    std::string filename = argv[1];

    std::cout << "Executing Graph: " << filename << std::endl;

    std::vector<Task> tasks = loadInput(filename);
    if (tasks.empty()) {
        std::cerr << "Task list empty or missing." << std::endl;
        return 1;
    }

    Dependents_Map dependents;
    Indegree_Map indegree;
    buildGraph(tasks, dependents, indegree);

    if (hasCycle(dependents)) {
        std::cerr << "Error: Deadlock Detected! Circular dependencies found." << std::endl;
        return 1; 
    }

    ThreadPool pool(std::thread::hardware_concurrency());
    runParallel(tasks, dependents, indegree, pool);

    std::cout << "Graph execution complete." << std::endl;

    //Print all failed tasks
    std::vector<int> failedTasks;
    for (const auto& task : tasks){
        if (task.failed){
            failedTasks.push_back(task.id);
        }
    }

    if (!failedTasks.empty()) {
        std::cout << "The following tasks failed or were skipped due to failure:" << std::endl;
        for (size_t i = 0; i < failedTasks.size(); ++i) {
            std::cout << failedTasks[i] << (i == failedTasks.size() - 1 ? "" : ", ");
        }
        std::cout << std::endl;
    } 
    else{
        std::cout << "All tasks finished successfully with zero failures." << std::endl;
    }
    return 0;
}