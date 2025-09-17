/*
 * Scheduler - A C++ implementation of the TO-BE-NAMED scheduler
 * Copyright (C) 2024
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <vector>
#include <queue>
#include <memory>
#include <chrono>

namespace scheduler {

/**
 * @brief Represents a task in the scheduler
 */
struct Task {
    int id;
    int priority;
    std::chrono::milliseconds duration;
    std::chrono::time_point<std::chrono::steady_clock> arrival_time;
    
    Task(int id, int priority, std::chrono::milliseconds duration);
};

/**
 * @brief Main scheduler class implementing various scheduling algorithms
 */
class Scheduler {
public:
    enum class Algorithm {
        FCFS,    // First Come First Served
        SJF,     // Shortest Job First
        PRIORITY, // Priority Scheduling
        RR       // Round Robin
    };

    explicit Scheduler(Algorithm algo = Algorithm::FCFS);
    ~Scheduler() = default;

    // Task management
    void addTask(std::unique_ptr<Task> task);
    void setAlgorithm(Algorithm algo);
    
    // Scheduling operations
    void run();
    void reset();
    
    // Statistics
    double getAverageWaitTime() const;
    double getAverageTurnaroundTime() const;
    size_t getCompletedTaskCount() const;

private:
    Algorithm current_algorithm_;
    std::queue<std::unique_ptr<Task>> task_queue_;
    std::vector<std::unique_ptr<Task>> completed_tasks_;
    
    // Algorithm implementations
    void runFCFS();
    void runSJF();
    void runPriority();
    void runRoundRobin();
    
    // Helper functions
    void executeTask(std::unique_ptr<Task> task);
    void calculateStatistics();
};

} // namespace scheduler

#endif // SCHEDULER_H