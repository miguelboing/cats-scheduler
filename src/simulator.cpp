/*
 * Simulator implementation
 * Copyright (C) 2024
 */

#include "simulator.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>

namespace scheduler {

Simulator::Simulator(const SimulationConfig& config) 
    : config_(config), random_generator_(std::chrono::steady_clock::now().time_since_epoch().count()) {}

void Simulator::runSimulation(Scheduler::Algorithm algorithm) {
    std::cout << "\n=== Running Simulation with ";
    switch (algorithm) {
        case Scheduler::Algorithm::FCFS: std::cout << "FCFS"; break;
        case Scheduler::Algorithm::SJF: std::cout << "SJF"; break;
        case Scheduler::Algorithm::PRIORITY: std::cout << "Priority"; break;
        case Scheduler::Algorithm::RR: std::cout << "Round Robin"; break;
    }
    std::cout << " algorithm ===" << std::endl;
    
    runSingleAlgorithm(algorithm);
}

void Simulator::compareAlgorithms() {
    std::cout << "\n=== Comparing All Scheduling Algorithms ===" << std::endl;
    
    results_.clear();
    
    // Test all algorithms
    runSingleAlgorithm(Scheduler::Algorithm::FCFS);
    runSingleAlgorithm(Scheduler::Algorithm::SJF);
    runSingleAlgorithm(Scheduler::Algorithm::PRIORITY);
    runSingleAlgorithm(Scheduler::Algorithm::RR);
    
    printComparison();
}

void Simulator::setConfig(const SimulationConfig& config) {
    config_ = config;
}

const SimulationConfig& Simulator::getConfig() const {
    return config_;
}

void Simulator::printResults() const {
    if (results_.empty()) {
        std::cout << "No simulation results to display." << std::endl;
        return;
    }
    
    const auto& result = results_.back();
    std::cout << "\n--- Simulation Results ---" << std::endl;
    std::cout << "Algorithm: ";
    switch (result.algorithm) {
        case Scheduler::Algorithm::FCFS: std::cout << "FCFS"; break;
        case Scheduler::Algorithm::SJF: std::cout << "SJF"; break;
        case Scheduler::Algorithm::PRIORITY: std::cout << "Priority"; break;
        case Scheduler::Algorithm::RR: std::cout << "Round Robin"; break;
    }
    std::cout << std::endl;
    std::cout << "Completed Tasks: " << result.completed_tasks << std::endl;
    std::cout << "Average Wait Time: " << std::fixed << std::setprecision(2) 
              << result.avg_wait_time << " ms" << std::endl;
    std::cout << "Average Turnaround Time: " << std::fixed << std::setprecision(2) 
              << result.avg_turnaround_time << " ms" << std::endl;
}

void Simulator::printComparison() const {
    if (results_.size() < 2) {
        std::cout << "Need at least 2 algorithm results for comparison." << std::endl;
        return;
    }
    
    std::cout << "\n--- Algorithm Comparison ---" << std::endl;
    std::cout << std::left << std::setw(12) << "Algorithm" 
              << std::setw(10) << "Tasks" 
              << std::setw(15) << "Avg Wait (ms)"
              << std::setw(20) << "Avg Turnaround (ms)" << std::endl;
    std::cout << std::string(57, '-') << std::endl;
    
    for (const auto& result : results_) {
        std::string algo_name;
        switch (result.algorithm) {
            case Scheduler::Algorithm::FCFS: algo_name = "FCFS"; break;
            case Scheduler::Algorithm::SJF: algo_name = "SJF"; break;
            case Scheduler::Algorithm::PRIORITY: algo_name = "Priority"; break;
            case Scheduler::Algorithm::RR: algo_name = "Round Robin"; break;
        }
        
        std::cout << std::left << std::setw(12) << algo_name
                  << std::setw(10) << result.completed_tasks
                  << std::setw(15) << std::fixed << std::setprecision(2) << result.avg_wait_time
                  << std::setw(20) << std::fixed << std::setprecision(2) << result.avg_turnaround_time
                  << std::endl;
    }
}

std::vector<std::unique_ptr<Task>> Simulator::generateTasks() {
    std::vector<std::unique_ptr<Task>> tasks;
    
    std::uniform_int_distribution<int> duration_dist(config_.min_duration_ms, config_.max_duration_ms);
    std::uniform_int_distribution<int> priority_dist(config_.min_priority, config_.max_priority);
    
    std::cout << "Generating " << config_.num_tasks << " tasks..." << std::endl;
    
    for (size_t i = 0; i < config_.num_tasks; ++i) {
        int duration = duration_dist(random_generator_);
        int priority = priority_dist(random_generator_);
        
        auto task = std::make_unique<Task>(static_cast<int>(i + 1), priority, 
                                          std::chrono::milliseconds(duration));
        
        std::cout << "Task " << task->id << ": priority=" << task->priority 
                  << ", duration=" << task->duration.count() << "ms" << std::endl;
        
        tasks.push_back(std::move(task));
        
        // Simulate arrival interval
        if (i < config_.num_tasks - 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Brief delay for arrival time difference
        }
    }
    
    return tasks;
}

void Simulator::runSingleAlgorithm(Scheduler::Algorithm algorithm) {
    Scheduler scheduler(algorithm);
    
    // Generate fresh tasks for each algorithm
    auto tasks = generateTasks();
    
    // Add tasks to scheduler
    for (auto& task : tasks) {
        scheduler.addTask(std::move(task));
    }
    
    // Run the scheduler
    auto start_time = std::chrono::steady_clock::now();
    scheduler.run();
    auto end_time = std::chrono::steady_clock::now();
    
    // Store results
    AlgorithmResult result;
    result.algorithm = algorithm;
    result.avg_wait_time = scheduler.getAverageWaitTime();
    result.avg_turnaround_time = scheduler.getAverageTurnaroundTime();
    result.completed_tasks = scheduler.getCompletedTaskCount();
    
    results_.push_back(result);
    
    auto execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "Execution time: " << execution_time.count() << "ms" << std::endl;
    
    printResults();
}

} // namespace scheduler