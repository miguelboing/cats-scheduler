/*
 * Scheduler implementation
 * Copyright (C) 2024
 */

#include "scheduler.h"
#include <iostream>
#include <algorithm>
#include <thread>

namespace scheduler {

Task::Task(int id, int priority, std::chrono::milliseconds duration)
    : id(id), priority(priority), duration(duration), 
      arrival_time(std::chrono::steady_clock::now()) {}

Scheduler::Scheduler(Algorithm algo) 
    : current_algorithm_(algo) {}

void Scheduler::addTask(std::unique_ptr<Task> task) {
    task_queue_.push(std::move(task));
}

void Scheduler::setAlgorithm(Algorithm algo) {
    current_algorithm_ = algo;
}

void Scheduler::run() {
    switch (current_algorithm_) {
        case Algorithm::FCFS:
            runFCFS();
            break;
        case Algorithm::SJF:
            runSJF();
            break;
        case Algorithm::PRIORITY:
            runPriority();
            break;
        case Algorithm::RR:
            runRoundRobin();
            break;
    }
    calculateStatistics();
}

void Scheduler::reset() {
    // Clear all tasks
    while (!task_queue_.empty()) {
        task_queue_.pop();
    }
    completed_tasks_.clear();
}

double Scheduler::getAverageWaitTime() const {
    if (completed_tasks_.empty()) return 0.0;
    
    double total_wait_time = 0.0;
    for (const auto& task : completed_tasks_) {
        // Simplified wait time calculation
        total_wait_time += task->duration.count() * 0.5; // Placeholder calculation
    }
    return total_wait_time / completed_tasks_.size();
}

double Scheduler::getAverageTurnaroundTime() const {
    if (completed_tasks_.empty()) return 0.0;
    
    double total_turnaround_time = 0.0;
    for (const auto& task : completed_tasks_) {
        // Simplified turnaround time calculation
        total_turnaround_time += task->duration.count();
    }
    return total_turnaround_time / completed_tasks_.size();
}

size_t Scheduler::getCompletedTaskCount() const {
    return completed_tasks_.size();
}

void Scheduler::runFCFS() {
    std::cout << "Running FCFS (First Come First Served) algorithm..." << std::endl;
    
    while (!task_queue_.empty()) {
        auto task = std::move(const_cast<std::unique_ptr<Task>&>(task_queue_.front()));
        task_queue_.pop();
        executeTask(std::move(task));
    }
}

void Scheduler::runSJF() {
    std::cout << "Running SJF (Shortest Job First) algorithm..." << std::endl;
    
    // Convert queue to vector for sorting
    std::vector<std::unique_ptr<Task>> tasks;
    while (!task_queue_.empty()) {
        tasks.push_back(std::move(const_cast<std::unique_ptr<Task>&>(task_queue_.front())));
        task_queue_.pop();
    }
    
    // Sort by duration (shortest first)
    std::sort(tasks.begin(), tasks.end(), 
              [](const std::unique_ptr<Task>& a, const std::unique_ptr<Task>& b) {
                  return a->duration < b->duration;
              });
    
    // Execute sorted tasks
    for (auto& task : tasks) {
        executeTask(std::move(task));
    }
}

void Scheduler::runPriority() {
    std::cout << "Running Priority scheduling algorithm..." << std::endl;
    
    // Convert queue to vector for sorting
    std::vector<std::unique_ptr<Task>> tasks;
    while (!task_queue_.empty()) {
        tasks.push_back(std::move(const_cast<std::unique_ptr<Task>&>(task_queue_.front())));
        task_queue_.pop();
    }
    
    // Sort by priority (higher priority first)
    std::sort(tasks.begin(), tasks.end(), 
              [](const std::unique_ptr<Task>& a, const std::unique_ptr<Task>& b) {
                  return a->priority > b->priority;
              });
    
    // Execute sorted tasks
    for (auto& task : tasks) {
        executeTask(std::move(task));
    }
}

void Scheduler::runRoundRobin() {
    std::cout << "Running Round Robin algorithm..." << std::endl;
    
    const std::chrono::milliseconds time_quantum{100}; // 100ms time slice
    std::queue<std::unique_ptr<Task>> round_robin_queue;
    
    // Move all tasks to round robin queue
    while (!task_queue_.empty()) {
        round_robin_queue.push(std::move(const_cast<std::unique_ptr<Task>&>(task_queue_.front())));
        task_queue_.pop();
    }
    
    while (!round_robin_queue.empty()) {
        auto task = std::move(round_robin_queue.front());
        round_robin_queue.pop();
        
        if (task->duration <= time_quantum) {
            // Task can finish in this time slice
            executeTask(std::move(task));
        } else {
            // Task needs more time, execute partial and requeue
            std::cout << "Executing task " << task->id << " for " << time_quantum.count() << "ms..." << std::endl;
            std::this_thread::sleep_for(time_quantum);
            task->duration -= time_quantum;
            round_robin_queue.push(std::move(task));
        }
    }
}

void Scheduler::executeTask(std::unique_ptr<Task> task) {
    std::cout << "Executing task " << task->id << " (priority: " << task->priority 
              << ", duration: " << task->duration.count() << "ms)..." << std::endl;
    
    // Simulate task execution
    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Brief simulation
    
    completed_tasks_.push_back(std::move(task));
}

void Scheduler::calculateStatistics() {
    // Statistics are calculated on-demand in getter methods
    std::cout << "Scheduling completed. " << completed_tasks_.size() << " tasks processed." << std::endl;
}

} // namespace scheduler