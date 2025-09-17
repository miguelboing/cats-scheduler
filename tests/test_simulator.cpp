/*
 * Google Test unit tests for Simulator
 * Copyright (C) 2024
 */

#include <gtest/gtest.h>
#include "simulator.h"

class SimulatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.num_tasks = 3;
        config_.min_duration_ms = 50;
        config_.max_duration_ms = 150;
        config_.min_priority = 1;
        config_.max_priority = 5;
        
        simulator_ = std::make_unique<scheduler::Simulator>(config_);
    }
    
    scheduler::SimulationConfig config_;
    std::unique_ptr<scheduler::Simulator> simulator_;
};

TEST_F(SimulatorTest, ConfigurationAccess) {
    EXPECT_EQ(simulator_->getConfig().num_tasks, 3);
    EXPECT_EQ(simulator_->getConfig().min_duration_ms, 50);
    EXPECT_EQ(simulator_->getConfig().max_duration_ms, 150);
}

TEST_F(SimulatorTest, ConfigurationChange) {
    scheduler::SimulationConfig new_config;
    new_config.num_tasks = 5;
    new_config.min_duration_ms = 100;
    new_config.max_duration_ms = 200;
    
    simulator_->setConfig(new_config);
    
    EXPECT_EQ(simulator_->getConfig().num_tasks, 5);
    EXPECT_EQ(simulator_->getConfig().min_duration_ms, 100);
    EXPECT_EQ(simulator_->getConfig().max_duration_ms, 200);
}

TEST_F(SimulatorTest, SingleSimulation) {
    // This test just ensures the simulation runs without crashing
    EXPECT_NO_THROW(simulator_->runSimulation(scheduler::Scheduler::Algorithm::FCFS));
}

TEST_F(SimulatorTest, AlgorithmComparison) {
    // This test ensures comparison runs without crashing
    EXPECT_NO_THROW(simulator_->compareAlgorithms());
}