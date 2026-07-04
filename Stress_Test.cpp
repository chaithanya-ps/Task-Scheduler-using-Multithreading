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

const int MAX_TASKS = 501;
const int MAX_RUNS = 100;

std::atomic<int> exec_count[MAX_TASKS]; // to keep track of number of times the task is executed
bool errors[MAX_TASKS]; // true if tasks end up being permanently failed/skipped
bool failed_dep[MAX_TASKS]; // true if it has a failed dependency
int retryLimit[MAX_TASKS]; // Maximum number of reties allowed for a task

/*
    Generate a DAG, with some of them having a chance of being failed
    Use random values to determine whether there has to be dependency
    Run loop only from 1 to i so that graph must be DAG
    Allow some number of reties for all tasks alloted randomly (atmost 3)
    Allot retries irrespective of whether the the task passed/failed initially

*/
std::vector <Task> generateRandomDAG(int num_Tasks, double edge_prob, double fail_prob){
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution <double> rand(0.0, 1.0);
    std::uniform_int_distribution <int> retry(0, 4);    

    std::vector <Task> tasks;
    tasks.reserve(num_Tasks);

    for(int i = 0; i < MAX_TASKS; i++){
        exec_count[i].store(0, std::memory_order_relaxed);
        errors[i] = false;
        failed_dep[i] = false;
        retryLimit[i] = 0;
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
        int maxRetry = retry(gen);

        if(has_failed_dep || self_fail){
            errors[i] = true;
        }

        failed_dep[i] = has_failed_dep;
        retryLimit[i] = maxRetry;

        auto work = [i, self_fail](){
            exec_count[i].fetch_add(1, std::memory_order_relaxed);
            return !self_fail;
        };
        tasks.emplace_back(i, dependencies, work, maxRetry);
    }

    return tasks;
}

/*  
    Use Random probabilities to create a DAG 
    Make the graphs with Fail Probability = Edge Probability / 10
    Increment the exec_count[i] when any threads executes the task
    If any task is found to have executed > 1 time,, print that Race condition arouse and exit from the program

    Expected Execution Count for different types of tasks:
        Skipped (Failed Dependency)     = 0
        Self Failing                    = retryLimit[i] + 1
        Healthy                         = 1
    
    If it runs any number of times other than these given values, it means a Race condition arouse
*/

void runTest(int numTasks, int numRuns){
    assert(numTasks < MAX_TASKS);
    assert(numRuns <= MAX_RUNS);

    ThreadPool pool(std::thread::hardware_concurrency());
    for(int i = 1; i <= numRuns; i++){
        
        std::cout << std::endl << "Statring Run " << i << std::endl;

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution <double> rand(0.0, 0.5);
        
        double edge_prob = rand(gen);
        double fail_prob = edge_prob / 10;
        std::vector <Task> tasks = generateRandomDAG(numTasks, edge_prob, fail_prob);

        Dependents_Map dependents;
        Indegree_Map indegree;
        buildGraph(tasks, dependents, indegree);

        assert(!hasCycle(dependents) && "A cycle was present. Aborting execution..."); //Make sure graph is a DAG

        runParallel(tasks, dependents, indegree, pool);

        int tot_exec = 0;
        int tot_skip = 0;
        int tot_retry_failed = 0;

        for(int i = 1; i <= numTasks; i++){
            int count = exec_count[i].load(std::memory_order_relaxed);

            if(failed_dep[i]){
                if(count != 0){
                    std::cerr << "Race/Bug Detected. Skipped Task " << i << " executed " << 
                                  count << " times (expected 0)" << std::endl;
                    exit(1);
                }
                tot_skip++;
            }
            else if(errors[i]){
                int retry_max = retryLimit[i] + 1; 
                if(count != retry_max){
                    std::cerr << "Race/Bug Detected. Self-failing Task " << i << " executed " << 
                                  count << " times (expected " << retry_max << ")" << std::endl;
                    exit(1);
                }
                tot_retry_failed++;
            }
            else{
                if(count != 1){
                    std::cerr << "Race/Bug Detected. Healthy Task " << i << " executed " << 
                                  count << " times (expected 1)" << std::endl;
                    exit(1);
                }
                tot_exec++;
            }
        }
        std::cout << "Healthy                    = " << tot_exec << std::endl;
        std::cout << "Skipped                    = " << tot_skip << std::endl;
        std::cout << "Self-failed (with retries) = " << tot_retry_failed << std::endl;
    }   

}

//User needs to specify number of tasks and number of runs for runningly simulations easily

int main(){

    int numTasks = 0;
    int numRuns = 0;
    std::cout << "Enter number of tasks you want to simulate (atmost 500) : ";
    std::cin >> numTasks;
    std::cout << "Enter number of runs you wnt to simulate over (atmost 100): ";
    std::cin >> numRuns;
    runTest(numTasks, numRuns);

    std::cout << "SUCCESSFUL" << std::endl;
    
    return 0;
}