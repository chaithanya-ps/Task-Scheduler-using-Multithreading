#include <vector>
#include <mutex>
#include <cassert>
#include <iostream>
#include <atomic>
#include <random>
#include "task.h"
#include "graph.h"
#include "logger.h"
#include "scheduler.h"

// Stress Tester File to run on random Larger graphs

const int MAX_TASKS = 505;
std::atomic<int> exec_count[MAX_TASKS];
bool errors[MAX_TASKS];


/*
    Generate a DAG, with some of them having a chance of being failed
    Use random values to determine whether there has to be dependency
    Run loop only from 1 to i so that graph must be DAG
*/
std::vector <Task> generateRandomDAG(int num_Tasks, double edge_prob, double fail_prob){
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution <double> rand(0.0, 1.0);

    std::vector <Task> tasks;
    tasks.reserve(num_Tasks);

    for(int i = 0; i < MAX_TASKS; i++){
        exec_count[i].store(0, std::memory_order_relaxed);
        errors[i] = false;
    }

    for(int i = 1; i <= num_Tasks; i++){
        std::vector <int> dependencies;
        bool has_failed_dep = false;

        for(int j = 1; j < i; j++){
            if(rand(gen) < edge_prob){
                dependencies.push_back(j);
                if(errors[j])
                    has_failed_dep = true;
            }
        }

        bool self_fail = (rand(gen) < fail_prob);

        if(has_failed_dep || self_fail){
            errors[i] = true;
        }

        auto work = [i, self_fail](){
            exec_count[i].fetch_add(1, std::memory_order_relaxed);
            return !self_fail;
        };
        tasks.emplace_back(i, dependencies, work);
    }

    return tasks;
}

/*  
    Use Random probabilities to create a DAG 
    Make half of the runs to be all Passing tasks (Fail Probability = 0)
    Make the other half of the graphs with Fail Probability = Edge Probability / 10
    Increment the exec_count[i] when any threads executes the task
    If any task is found to have executed > 1 time,, print that Race condition arouse and exit from the program
*/

void runTest(int numTasks, int numRuns){
    assert(numTasks < MAX_TASKS);

    ThreadPool pool(std::thread::hardware_concurrency());
    for(int i = 1; i <= numRuns; i++){
        
        std::cout << std::endl << "Statring Run " << i << std::endl;

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution <double> rand(0.0, 0.5);
        
        double edge_prob = rand(gen);
        double fail_prob = (i <= numRuns / 2) ? 0 : edge_prob / 10;
        std::vector <Task> tasks = generateRandomDAG(numTasks, edge_prob, fail_prob);

        Dependents_Map dependents;
        Indegree_Map indegree;
        buildGraph(tasks, dependents, indegree);

        assert(!hasCycle(dependents));

        runParallel(tasks, dependents, indegree, pool);

        int tot_exec = 0;
        int tot_skip = 0;

        for(int i = 1; i <= numTasks; i++){
            int count = exec_count[i].load(std::memory_order_relaxed);
            if(errors[i]){
                if(count > 1){
                    std::cerr << "Race/Bug Detected. Failed Task " << i << " executed " << count << " times " << std::endl;
                    exit(1);
                }
                tot_skip++;
            }
            else{
                if(count != 1){
                    std::cerr << "Race/Bug Detected. Healthy Task " << i << " executed " << count << " times " << std::endl;
                    exit(1);
                }
                tot_exec++;
            }
        }
    }

}

int main(){

    runTest(500, 100);

    std::cout << "SUCCESSFUL" << std::endl;
    
    return 0;
}