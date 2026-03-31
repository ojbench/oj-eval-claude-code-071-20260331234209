#ifndef SRC_HPP
#define SRC_HPP

#include <vector>
#include <algorithm>
#include <queue>
#include <map>
#include <cmath>

// Basic task structure
struct Task {
    int id;
    int workload;
    int deadline;
    int priority;
    int arrival_time;
    int remaining_work;

    Task(int i, int w, int d, int p, int t = 0)
        : id(i), workload(w), deadline(d), priority(p), arrival_time(t), remaining_work(w) {}
};

// Container structure
struct Container {
    int task_id;
    int servers;
    int start_time;
    int stage; // 0: startup, 1: execution, 2: saving
    int stage_remaining;
    int work_done_this_run;

    Container(int tid, int s, int st, int stage_rem = 0)
        : task_id(tid), servers(s), start_time(st), stage(0),
          stage_remaining(stage_rem), work_done_this_run(0) {}
};

// Global constants (these would typically come from definition.h)
constexpr int kStartUp = 1;
constexpr int kSaving = 1;
constexpr double c = 1.0; // power constant

// Global state for scheduler
std::map<int, Task> all_tasks;
std::vector<Container> running_containers;
int current_time = 0;
int total_servers = 0;

// Client generator function
std::vector<Task> generate_tasks(int num_servers, int num_tasks,
                                 int min_deadline, int max_deadline,
                                 int min_workload, int max_workload,
                                 int min_priority, int max_priority) {
    std::vector<Task> tasks;

    // Generate simple tasks
    for (int i = 0; i < num_tasks; i++) {
        int workload = min_workload + (rand() % (max_workload - min_workload + 1));
        int deadline = min_deadline + (rand() % (max_deadline - min_deadline + 1));
        int priority = min_priority + (rand() % (max_priority - min_priority + 1));

        tasks.emplace_back(i, workload, deadline, priority);
    }

    return tasks;
}

// Server scheduler function
std::vector<int> schedule_tasks(int time, const std::vector<Task>& new_tasks, int num_servers) {
    current_time = time;
    total_servers = num_servers;

    // Add new tasks to global state
    for (const auto& task : new_tasks) {
        all_tasks[task.id] = task;
    }

    // Simple greedy scheduling: prioritize by priority/time ratio
    std::vector<int> schedule;

    // Calculate available servers
    int used_servers = 0;
    for (const auto& container : running_containers) {
        used_servers += container.servers;
    }
    int available_servers = num_servers - used_servers;

    // Sort tasks by priority and deadline
    std::vector<std::pair<int, int>> task_priority;
    for (const auto& [id, task] : all_tasks) {
        if (task.remaining_work > 0 && task.deadline > time) {
            int urgency = task.priority * 1000 / (task.deadline - time + 1);
            task_priority.emplace_back(urgency, id);
        }
    }
    std::sort(task_priority.rbegin(), task_priority.rend());

    // Schedule high priority tasks
    for (const auto& [urgency, task_id] : task_priority) {
        if (available_servers > 0) {
            int servers_to_use = std::min(available_servers,
                                         (int)std::sqrt(all_tasks[task_id].remaining_work));
            if (servers_to_use > 0) {
                schedule.push_back(task_id);
                schedule.push_back(servers_to_use);
                available_servers -= servers_to_use;

                // Start a new container
                running_containers.emplace_back(task_id, servers_to_use, time, kStartUp);
            }
        }
    }

    return schedule;
}

#endif // SRC_HPP
