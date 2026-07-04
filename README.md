# Parallel Task Scheduler

A multithreaded C++ task scheduler that runs independent tasks in parallel while respecting dependency ordering — similar to simplified `make`. Tasks can depend on other tasks, run concurrently across a thread pool when their dependencies allow it, retry on failure, and cleanly propagate failure to everything downstream when they don't.

Built as a small project to explore real C++ concurrency: `std::thread`, `std::mutex`, `std::condition_variable`, `std::atomic`, thread pools, and dependency-graph scheduling — rather than just using higher-level concurrency wrappers.

## Features

- **Dependency-based parallel execution** — tasks run as soon as all their dependencies finish, not in a fixed precomputed order
- **Cycle detection** — invalid (circular) dependency graphs are rejected before anything runs
- **Custom thread pool** — built from scratch with `std::thread`, `std::mutex`, and `std::condition_variable`, not a library
- **Failure propagation** — if a task fails, every task that depends on it (transitively) is automatically skipped, not run
- **Retry logic** — a failing task can be configured to retry up to N times before its failure is propagated downstream
- **Structured logging** — every task's start/completion/failure/retry/skip is logged with a timestamp and thread ID
- **Stress-tested** — a dedicated stress test generates random large graphs (hundreds of tasks) with random failures and retries, and verifies every task ran *exactly* the expected number of times, catching races that only show up under real concurrent load

## How it works

1. **Parse** the task file into a list of tasks, each with an ID, a unit of work, a list of dependency IDs, and an optional retry limit
2. **Build the dependency graph** — for each task, record who depends on it (`Dependents_Map`) and how many unfinished dependencies it has (`Indegree_Map`)
3. **Check for cycles** — a three-state DFS (unvisited / visiting / visited) rejects any graph with a circular dependency before scheduling starts
4. **Seed the thread pool** with every task that has zero dependencies
5. **Run**: each worker thread executes a task, then decrements the "pending" count of everything that depends on it. Any dependent whose count hits zero gets submitted to the pool immediately — no fixed schedule, readiness drives execution
6. **On failure**: a task retries up to its configured limit; once exhausted, it's marked failed and that status cascades to every downstream dependent, which get skipped rather than run

## Build
 
Two separate makefiles — one for the main scheduler, one for the stress tester (built with ThreadSanitizer enabled):
 
```bash
make                                # builds task_scheduler
make test                           # runs every tests/*.txt file through it
make clean                          # remove task_scheduler
 
make -f make_stress_tester.mk       # builds stress_tester (with -fsanitize=thread)
make -f make_stress_tester.mk test  # runs the stress test
make -f make_stress_tester.mk clean # remove stress_tester
```
 
Both makefiles track the shared headers (`task.h`, `graph.h`, `scheduler.h`, `logger.h`) as dependencies, so each only recompiles when something it depends on has actually changed.
 
## To run on a specific input
**Compilation**
```bash
g++ -std=c++17 -Wall -Wextra -Wreorder -O2 main.cpp -o task_scheduler
```

**To Run**
```bash
./task_scheduler <task_file>
```
### Task file format

```
id1 [F|P] [Rn] dep1 dep2 ...
```

| Field | Meaning | Default |
|---|---|---|
| `id` | Task ID (integer) | required |
| `F` / `P` | Whether the task's own work fails (`F`) or passes (`P`) | `P` |
| `Rn` | Retry limit — e.g. `R2` allows 2 retries (3 total attempts) before the task is marked permanently failed | `R0` |
| `dep1 dep2 ...` | IDs of tasks this one depends on | none |

The flag and retry token can appear in either order, and both are optional — older task files without them still work unchanged.

**Example:**
```
1 P
2 F R2 1
3 P 1
4 P 2 3
```
Task 1 runs and passes. Task 2 depends on task 1, fails, retries twice, then is marked failed. Task 3 (also depending on 1) runs fine. Task 4 depends on both 2 and 3 — since 2 ultimately failed, task 4 is skipped, even though task 3 succeeded.

### Sample output

```
Executing Graph: ./tests/sample
16:52:21 	Thread ID : 131973091628736	[STARTING] 	 Task 1 starting
16:52:21 	Thread ID : 131973091628736	[RUNNING] 	 Task 1 running
16:52:21 	Thread ID : 131973091628736	[DONE]    	 Task 1 completed
16:52:21 	Thread ID : 131973091628736	[STARTING] 	 Task 2 starting
16:52:21 	Thread ID : 131973091628736	[RUNNING] 	 Task 2 running
16:52:21 	Thread ID : 131973091628736	[RETRY]   	 Task 2 failed, retrying (attempt 1/2)
16:52:21 	Thread ID : 131973091628736	[STARTING] 	 Task 2 starting
16:52:21 	Thread ID : 131973091628736	[RUNNING] 	 Task 2 running
16:52:21 	Thread ID : 131973091628736	[RETRY]   	 Task 2 failed, retrying (attempt 2/2)
16:52:21 	Thread ID : 131973091628736	[STARTING] 	 Task 2 starting
16:52:21 	Thread ID : 131973091628736	[RUNNING] 	 Task 2 running
16:52:21 	Thread ID : 131973091628736	[FAILED]  	 Task 2 failed (out of retries)
16:52:21 	Thread ID : 131973200733888	[STARTING] 	 Task 3 starting
16:52:21 	Thread ID : 131973200733888	[RUNNING] 	 Task 3 running
16:52:21 	Thread ID : 131973200733888	[DONE]    	 Task 3 completed
16:52:21 	Thread ID : 131973200733888	[SKIPPED] 	 Task 4 skipped
Graph execution complete.
The following tasks failed or were skipped due to failure:
2, 4
```

Note that only the task which actually ran and exhausted its retries logs `FAILED` — every downstream dependent logs `SKIPPED`, since it never got the chance to run its own work at all.

## Project structure

```
main.cpp              # entry point, task file loader, CLI
task.h                # Task struct
graph.h               # dependency graph construction + cycle detection
scheduler.h           # ThreadPool, sequential + parallel execution, failure propagation
logger.h              # thread-safe, timestamped logging
Stress_Test.cpp       # randomized large-scale concurrency test
Makefile              # builds task_scheduler, plus a test convenience target
make_stress_tester.mk # builds stress_tester (with ThreadSanitizer), plus a test target
tests/                # hand-written test cases covering specific scenarios
    test01-07         # valid DAGs of increasing structural complexity
    test03            # deliberate cycle (should be rejected)
    test08-10         # failure propagation, diamond-shaped failures, retries
```

## Testing

**Unit-style tests** (`tests/`) cover specific scenarios: linear chains, diamond dependencies, deliberate cycles, single and multiple failure points, and retry behavior.

**Stress test** (`Stress_Test.cpp`) generates random DAGs (up to 500 tasks) with random failure and retry probabilities, across upto 100 runs, and asserts every task executed *exactly* the number of times expected given its position in the graph — catching duplicate executions, missed executions, and races that deterministic tests won't surface.

```bash
g++ -std=c++17 -Wall -Wextra -pthread Stress_Test.cpp -o stress_test
./stress_test
```

## What I learnt building this

Learnt a lot of new things doing this project:
- **Multithreading**: Basics of Multithreading. What is the concept invloved
- **Different types of concepts invloved in implementation** : Different types of Mutex, Locks, Condition Variables, DeadLock, Atomic Variables
- **Lambdas** : How to implement lambdas for concurrency
