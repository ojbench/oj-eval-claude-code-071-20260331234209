#pragma once
#include <algorithm>
#include <vector>
#include <cmath>
#include <map>
#include <random>

namespace oj {

// Simple, safe task generator
auto generate_tasks(const Description &desc) -> std::vector<Task> {
    std::vector<Task> tasks;
    tasks.reserve(desc.task_count);

    std::mt19937_64 rng(42);

    // Use middle values for sums
    time_t target_exec_sum = (desc.execution_time_sum.min + desc.execution_time_sum.max) / 2;
    priority_t target_prio_sum = (desc.priority_sum.min + desc.priority_sum.max) / 2;

    // Distribute evenly
    time_t base_exec = target_exec_sum / desc.task_count;
    priority_t base_prio = target_prio_sum / desc.task_count;

    time_t actual_exec_sum = 0;
    priority_t actual_prio_sum = 0;

    double max_cpu_power = std::pow((double)PublicInformation::kCPUCount, PublicInformation::kAccel);

    for (size_t i = 0; i < desc.task_count; ++i) {
        time_t exec_time, deadline, launch_time;
        priority_t priority;

        if (i < desc.task_count - 1) {
            exec_time = std::clamp(base_exec, desc.execution_time_single.min, desc.execution_time_single.max);
            priority = std::clamp(base_prio, desc.priority_single.min, desc.priority_single.max);
        } else {
            // Last task adjusts to hit target sum exactly
            exec_time = target_exec_sum - actual_exec_sum;
            exec_time = std::clamp(exec_time, desc.execution_time_single.min, desc.execution_time_single.max);

            priority = target_prio_sum - actual_prio_sum;
            priority = std::clamp(priority, desc.priority_single.min, desc.priority_single.max);
        }

        actual_exec_sum += exec_time;
        actual_prio_sum += priority;

        // Calculate minimum time to complete
        time_t min_time = (time_t)std::ceil(
            PublicInformation::kStartUp +
            PublicInformation::kSaving +
            exec_time / max_cpu_power
        ) + 2; // Add 2 for safety

        // Simple: launch all tasks at time 0
        launch_time = 0;

        // Set deadline with sufficient slack
        deadline = std::clamp(min_time + 100, desc.deadline_time.min, desc.deadline_time.max);

        tasks.push_back(Task{
            .launch_time = launch_time,
            .deadline = deadline,
            .execution_time = exec_time,
            .priority = priority
        });
    }

    return tasks;
}

} // namespace oj

namespace oj {

// Scheduler state
struct TaskInfo {
    time_t execution_time;
    time_t deadline;
    priority_t priority;
    double work_done;
    bool completed;
    bool running;
    cpu_id_t current_cpus;
    time_t launch_start;
};

static std::map<task_id_t, TaskInfo> all_tasks;
static std::vector<task_id_t> running_tasks;

auto schedule_tasks(time_t time, std::vector<Task> list, const Description &desc) -> std::vector<Policy> {
    static task_id_t task_id = 0;
    const task_id_t first_id = task_id;

    std::vector<Policy> policies;

    // Add new tasks to our state
    for (size_t i = 0; i < list.size(); ++i) {
        const auto &task = list[i];
        all_tasks[first_id + i] = TaskInfo{
            .execution_time = task.execution_time,
            .deadline = task.deadline,
            .priority = task.priority,
            .work_done = 0.0,
            .completed = false,
            .running = false,
            .current_cpus = 0,
            .launch_start = 0
        };
    }

    task_id += list.size();

    // Check running tasks and decide whether to save or cancel
    std::vector<task_id_t> still_running;
    cpu_id_t used_cpus = 0;

    for (auto tid : running_tasks) {
        auto &info = all_tasks[tid];
        if (!info.running) continue;

        // Calculate work done so far
        time_t duration = time - info.launch_start;
        double work = time_policy(duration, info.current_cpus);

        // Decide: should we save this task?
        bool should_save = false;

        if (info.work_done + work >= info.execution_time) {
            // Task completed
            should_save = true;
        } else if (time + PublicInformation::kSaving >= info.deadline) {
            // Not enough time to complete, save what we have
            should_save = true;
        } else if (duration >= PublicInformation::kStartUp + 10) {
            // Save checkpoints periodically
            should_save = (duration % 20 == 0);
        }

        if (should_save && duration >= PublicInformation::kStartUp) {
            policies.push_back(Saving{.task_id = tid});
            info.work_done += work;
            info.running = false;

            if (info.work_done >= info.execution_time) {
                info.completed = true;
            }
        } else {
            still_running.push_back(tid);
            used_cpus += info.current_cpus;
        }
    }

    running_tasks = still_running;

    // Priority queue: schedule urgent, high-priority tasks
    std::vector<std::pair<double, task_id_t>> candidates;

    for (const auto &[tid, info] : all_tasks) {
        if (info.completed || info.running) continue;
        if (info.deadline <= time) continue;

        double remaining_work = info.execution_time - info.work_done;
        if (remaining_work <= 0) continue;

        time_t time_left = info.deadline - time - PublicInformation::kStartUp - PublicInformation::kSaving;
        if (time_left <= 0) continue;

        // Priority score: higher priority, less time left, more urgent
        double urgency = (double)info.priority / (time_left + 1) * remaining_work;
        candidates.push_back({urgency, tid});
    }

    // Sort by urgency (descending)
    std::sort(candidates.begin(), candidates.end(), std::greater<>());

    // Launch tasks with available CPUs
    for (const auto &[urgency, tid] : candidates) {
        if (used_cpus >= desc.cpu_count) break;

        auto &info = all_tasks[tid];
        double remaining_work = info.execution_time - info.work_done;
        time_t time_left = info.deadline - time - PublicInformation::kStartUp - PublicInformation::kSaving;

        // Calculate optimal CPU allocation
        double min_power = remaining_work / time_left;
        cpu_id_t min_cpus = std::max(1UL, (cpu_id_t)std::ceil(std::pow(min_power, 1.0 / PublicInformation::kAccel)));

        // Don't use too many CPUs (diminishing returns)
        cpu_id_t target_cpus = std::min(min_cpus, (cpu_id_t)20);
        target_cpus = std::min(target_cpus, desc.cpu_count - used_cpus);

        if (target_cpus > 0) {
            policies.push_back(Launch{.cpu_cnt = target_cpus, .task_id = tid});
            info.running = true;
            info.current_cpus = target_cpus;
            info.launch_start = time;
            running_tasks.push_back(tid);
            used_cpus += target_cpus;
        }
    }

    return policies;
}

} // namespace oj
