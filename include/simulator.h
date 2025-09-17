/*
 * Simulator - A simulation environment for the scheduler
 * Copyright (C) 2024
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "scheduler.h"
#include <random>
#include <vector>

namespace scheduler {

/**
 * @brief Simulation parameters for generating tasks
 */
struct SimulationConfig {
    size_t num_tasks = 10;
    int min_duration_ms = 100;
    int max_duration_ms = 1000;
    int min_priority = 1;
    int max_priority = 10;
    std::chrono::milliseconds arrival_interval{100};
};

/**
 * @brief Simulator class for testing scheduler algorithms
 */
class Simulator {
public:
    explicit Simulator(const SimulationConfig& config = SimulationConfig{});
    ~Simulator() = default;

    // Simulation control
    void runSimulation(Scheduler::Algorithm algorithm);
    void compareAlgorithms();
    
    // Configuration
    void setConfig(const SimulationConfig& config);
    const SimulationConfig& getConfig() const;
    
    // Results
    void printResults() const;
    void printComparison() const;

private:
    SimulationConfig config_;
    std::mt19937 random_generator_;
    
    struct AlgorithmResult {
        Scheduler::Algorithm algorithm;
        double avg_wait_time;
        double avg_turnaround_time;
        size_t completed_tasks;
    };
    
    std::vector<AlgorithmResult> results_;
    
    // Helper functions
    std::vector<std::unique_ptr<Task>> generateTasks();
    void runSingleAlgorithm(Scheduler::Algorithm algorithm);
};

} // namespace scheduler

#endif // SIMULATOR_H