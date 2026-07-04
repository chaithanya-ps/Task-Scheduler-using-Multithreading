#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <mutex>
#include <thread>
#include <algorithm>
#include <cctype>
#include "logger.h"
#include "task.h"
#include "graph.h"
#include "scheduler.h"


auto makeWork(int id, bool shouldFail){
    return [id, shouldFail]() {
        {
           logMessage("[RUNNING] \t Task " + std::to_string(id) + " running");
        }
        return !shouldFail;
    };
}

/*
    Parses input file which contains details of the graph
    Expected format of lines in the file
    [ID] [P/F] [R<n>] [Dependency 1] [Dependency 2] ....

    P/F : Pass or Fail
    R<n> : Number of retries allowed

    [P/F], [R<n>] not compulsory
*/
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

        std::string flag = "P"; // Default to PASS
        int retry = 0; // Default number of retries allowed to 0
        std::string nextToken; 
        
        while (ss >> nextToken) { // Read all tokens

            // If nextToken is F/P read it to PASS/FAIL
            if (nextToken == "F" || nextToken == "P") {
                flag = nextToken;
            } 
            
            // If nextToken is R<n>, read the maximum number of retries
            else if(nextToken.size() > 1 && nextToken[0] == 'R' &&
                    std::all_of(nextToken.begin() + 1, nextToken.end(), ::isdigit)) { 
                        retry = std::stoi(nextToken.substr(1));
                }

            // Otherwise, stream pointer is moved to start of the Token
            else { 
                ss.seekg(-static_cast<std::streamoff>(nextToken.size()), std::ios_base::cur);
                break;
            }
        }

        bool shouldFail = (flag == "F");

        std::vector<int> dependencies;
        int dep_id;
        while (ss >> dep_id) {
            dependencies.push_back(dep_id);
        }

        tasks.emplace_back(id, dependencies, makeWork(id, shouldFail), retry);
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
    buildGraph(tasks, dependents, indegree); // Build Graph with given dependency list

    if (hasCycle(dependents)) { //Make sure no Cycle is present in the Graph
        std::cerr << "Error: Deadlock Detected! Circular dependencies found." << std::endl;
        return 1; 
    }

    ThreadPool pool(std::thread::hardware_concurrency()); // Runs on any system now
    runParallel(tasks, dependents, indegree, pool); // Run with Multiple threads

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