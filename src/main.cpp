/*
 * Main application for the C++ Scheduler
 * Copyright (C) 2024
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "scheduler.h"
#include "simulator.h"
#include <iostream>
#include <memory>

void demonstrateBasicScheduler() {
    std::cout << "=== Basic Scheduler Demonstration ===" << std::endl;
    
    scheduler::Scheduler sched(scheduler::Scheduler::Algorithm::FCFS);
    
    // Create some sample tasks
    sched.addTask(std::make_unique<scheduler::Task>(1, 5, std::chrono::milliseconds(300)));
    sched.addTask(std::make_unique<scheduler::Task>(2, 3, std::chrono::milliseconds(150)));
    sched.addTask(std::make_unique<scheduler::Task>(3, 8, std::chrono::milliseconds(400)));
    sched.addTask(std::make_unique<scheduler::Task>(4, 1, std::chrono::milliseconds(200)));
    
    // Run the scheduler
    sched.run();
    
    // Display results
    std::cout << "\nResults:" << std::endl;
    std::cout << "Completed tasks: " << sched.getCompletedTaskCount() << std::endl;
    std::cout << "Average wait time: " << sched.getAverageWaitTime() << " ms" << std::endl;
    std::cout << "Average turnaround time: " << sched.getAverageTurnaroundTime() << " ms" << std::endl;
}

void runSimulation() {
    std::cout << "\n=== Scheduler Simulation ===" << std::endl;
    
    // Configure simulation
    scheduler::SimulationConfig config;
    config.num_tasks = 5;
    config.min_duration_ms = 100;
    config.max_duration_ms = 500;
    config.min_priority = 1;
    config.max_priority = 5;
    
    scheduler::Simulator sim(config);
    
    // Run simulation with different algorithms
    sim.runSimulation(scheduler::Scheduler::Algorithm::FCFS);
    sim.runSimulation(scheduler::Scheduler::Algorithm::SJF);
    sim.runSimulation(scheduler::Scheduler::Algorithm::PRIORITY);
    
    // Compare all algorithms
    sim.compareAlgorithms();
}

void printWelcome() {
    std::cout << "╔══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                    C++ Scheduler Demo                       ║" << std::endl;
    std::cout << "║                                                              ║" << std::endl;
    std::cout << "║  An algorithmic description in C++ of scheduling            ║" << std::endl;
    std::cout << "║  algorithms along with a simulator.                         ║" << std::endl;
    std::cout << "║                                                              ║" << std::endl;
    std::cout << "║  Supported Algorithms:                                       ║" << std::endl;
    std::cout << "║  • FCFS (First Come First Served)                           ║" << std::endl;
    std::cout << "║  • SJF (Shortest Job First)                                 ║" << std::endl;
    std::cout << "║  • Priority Scheduling                                       ║" << std::endl;
    std::cout << "║  • Round Robin                                               ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;
}

int main() {
    printWelcome();
    
    try {
        // Demonstrate basic scheduler functionality
        demonstrateBasicScheduler();
        
        // Run comprehensive simulation
        runSimulation();
        
        std::cout << "\n=== Scheduler Demo Complete ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}