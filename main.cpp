#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include "task.h"
#include "graph.h"
#include "scheduler.h"

/*
  Parses a text file and converts it into a vector of Task structures.
  Format: [task_id] [dep_id_1] [dep_id_2] [dep_id_3] ...
 */
std::vector<Task> loadInput(const std::string& filename) {
    std::vector<Task> tasks;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Could not open file: " << filename << "\n";
        return tasks;
    }

    std::string line;
    while (std::getline(file, line)){
        if (line.empty()) 
            continue; 

        std::stringstream ss(line);
        int id;
        
        // Read the task_id first
        if (!(ss >> id)) 
            continue;

        std::vector<int> dependencies;
        int dep_id;
        while (ss >> dep_id) {
            dependencies.push_back(dep_id);
        }

        Task t(id, dependencies, [id]() {
            std::cout << "Task " << id << " complete.\n";
            return true;
        });

        tasks.emplace_back(std::move(t));
    }

    return tasks;
}

int main(int argc, char* argv[]){
    std::string filename; 
    if (argc > 1) {
        filename = argv[1];
    }

    // Parse and ingest tasks
    std::vector<Task> tasks = loadInput(filename);
    if (tasks.empty()) {
        std::cerr << "Task list empty or missing.\n";
        return 1;
    }
    std::cout << "Loaded " << tasks.size() << " tasks.\n";

    Dependents_Map dependents; //adjacency map
    Indegree_Map indegree; //indegree map

    buildGraph(tasks, dependents, indegree); // Constructing directed Graph

    // Check for Cycles
    if (hasCycle(dependents)) {
        std::cerr << "Deadlock Detected! Circular dependencies found.\n";
        return 1; 
    }
    std::cout << "No cycles found.\n";

    // // Sort and Execute
    // std::vector<int> order = topologicalSort(tasks, dependents, indegree);

    //Instantiate Thread Pool with desired number of threads
    ThreadPool pool(8);

    // std::cout << "Execution sequence: ";
    // for (size_t i = 0; i < order.size(); i++) {
    //     std::cout << order[i] << (i == order.size() - 1 ? "\n" : " -> ");
    // }
    // std::cout << std::endl;

    //runSequential(tasks, order);

    //We'll use runParallel to make use of Multithreading
    runParallel(tasks, dependents, indegree, pool);
    std::cout << "All tasks complete.\n";

    return 0; 
}