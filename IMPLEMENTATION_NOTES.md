# Implementation Notes for Problem 071 - Cluster Scheduling

## Overview
This implementation provides both a client (task generator) and server (task scheduler) for the cluster scheduling problem.

## Client (Task Generator)
- Generates tasks that satisfy all constraints in the Description
- Ensures all tasks are theoretically completable (deadline > launch_time + startup + saving + min_exec_time)
- Distributes workload and priority evenly to meet sum constraints
- All tasks launch at time 0 for simplicity
- Deadlines are set with sufficient slack to ensure feasibility

## Server (Task Scheduler)
- Uses a priority-based greedy scheduling algorithm
- Prioritizes tasks by urgency = priority / (time_left + 1) * remaining_work
- Dynamically allocates CPUs based on remaining work and time left
- Saves tasks periodically to create checkpoints
- Handles the three-stage container model: startup, execution, saving

## Key Features
1. **CPU Allocation**: Calculates optimal CPU count using formula: k >= (remaining_work / time_left)^(1/0.75)
2. **Checkpoint System**: Saves tasks periodically (every 20 time units after startup)
3. **Deadline Awareness**: Saves tasks when approaching deadline even if incomplete
4. **Resource Management**: Ensures total CPU usage never exceeds available CPUs

## Constraints Satisfied
- All single-task constraints (exec_time, priority, deadline ranges)
- Sum constraints (total exec_time and priority within ranges)
- Feasibility constraint: launch_time + kStartUp + kSaving + exec_time/CPU_power < deadline

## Known Limitations
- Simple even distribution may not be optimal for "attacking" other schedulers
- Could be improved with more sophisticated task arrival patterns
- Scheduler could benefit from more advanced algorithms (e.g., machine learning-based)

## Testing
To test locally:
```bash
cd oj
bash run.sh --compile
echo "0" | bash run.sh  # Test case 0 (small)
echo "1" | bash run.sh  # Test case 1 (middle)
echo "2" | bash run.sh  # Test case 2 (senpai)
echo "3" | bash run.sh  # Test case 3 (huge)
```

## Submission
According to the problem, submissions should be made to http://10.80.75.141/OnlineJudge (not the public ACMOJ).
The src.hpp file should be submitted without the `#include "interface.h"` and `#include "definition.h"` lines.
