/*
 * Simple test runner (fallback when GTest is not available)
 * Copyright (C) 2024
 */

#include "scheduler.h"
#include "simulator.h"
#include <iostream>
#include <cassert>

void test_task_creation() {
    std::cout << "Testing Task creation..." << std::endl;
    
    scheduler::Task task(1, 5, std::chrono::milliseconds(100));
    assert(task.id == 1);
    assert(task.priority == 5);
    assert(task.duration == std::chrono::milliseconds(100));
    
    std::cout << "✓ Task creation test passed" << std::endl;
}

void test_scheduler_creation() {
    std::cout << "Testing Scheduler creation..." << std::endl;
    
    scheduler::Scheduler sched(scheduler::Scheduler::Algorithm::FCFS);
    assert(sched.getCompletedTaskCount() == 0);
    
    std::cout << "✓ Scheduler creation test passed" << std::endl;
}

void test_basic_scheduling() {
    std::cout << "Testing basic scheduling..." << std::endl;
    
    scheduler::Scheduler sched(scheduler::Scheduler::Algorithm::FCFS);
    
    // Add a simple task
    sched.addTask(std::make_unique<scheduler::Task>(1, 5, std::chrono::milliseconds(50)));
    
    // Run scheduler
    sched.run();
    
    // Check results
    assert(sched.getCompletedTaskCount() == 1);
    assert(sched.getAverageTurnaroundTime() >= 0);
    
    std::cout << "✓ Basic scheduling test passed" << std::endl;
}

void test_simulator_creation() {
    std::cout << "Testing Simulator creation..." << std::endl;
    
    scheduler::SimulationConfig config;
    config.num_tasks = 3;
    
    scheduler::Simulator sim(config);
    assert(sim.getConfig().num_tasks == 3);
    
    std::cout << "✓ Simulator creation test passed" << std::endl;
}

int main() {
    std::cout << "=== Running Simple Tests ===" << std::endl;
    
    try {
        test_task_creation();
        test_scheduler_creation();
        test_basic_scheduling();
        test_simulator_creation();
        
        std::cout << "\n✓ All tests passed!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "✗ Test failed: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "✗ Test failed with unknown exception" << std::endl;
        return 1;
    }
}