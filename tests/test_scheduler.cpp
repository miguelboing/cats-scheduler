/*
 * Google Test unit tests for Scheduler
 * Copyright (C) 2024
 */

#include <gtest/gtest.h>
#include "scheduler.h"

class SchedulerTest : public ::testing::Test {
protected:
    void SetUp() override {
        scheduler_ = std::make_unique<scheduler::Scheduler>(scheduler::Scheduler::Algorithm::FCFS);
    }
    
    std::unique_ptr<scheduler::Scheduler> scheduler_;
};

TEST_F(SchedulerTest, InitialState) {
    EXPECT_EQ(scheduler_->getCompletedTaskCount(), 0);
    EXPECT_EQ(scheduler_->getAverageWaitTime(), 0.0);
    EXPECT_EQ(scheduler_->getAverageTurnaroundTime(), 0.0);
}

TEST_F(SchedulerTest, AddSingleTask) {
    auto task = std::make_unique<scheduler::Task>(1, 5, std::chrono::milliseconds(100));
    scheduler_->addTask(std::move(task));
    
    scheduler_->run();
    
    EXPECT_EQ(scheduler_->getCompletedTaskCount(), 1);
    EXPECT_GT(scheduler_->getAverageTurnaroundTime(), 0.0);
}

TEST_F(SchedulerTest, FCFSOrdering) {
    // Add tasks in specific order
    scheduler_->addTask(std::make_unique<scheduler::Task>(1, 1, std::chrono::milliseconds(100)));
    scheduler_->addTask(std::make_unique<scheduler::Task>(2, 10, std::chrono::milliseconds(50)));
    scheduler_->addTask(std::make_unique<scheduler::Task>(3, 5, std::chrono::milliseconds(200)));
    
    scheduler_->run();
    
    EXPECT_EQ(scheduler_->getCompletedTaskCount(), 3);
}

TEST_F(SchedulerTest, AlgorithmChange) {
    scheduler_->setAlgorithm(scheduler::Scheduler::Algorithm::SJF);
    
    scheduler_->addTask(std::make_unique<scheduler::Task>(1, 1, std::chrono::milliseconds(200)));
    scheduler_->addTask(std::make_unique<scheduler::Task>(2, 1, std::chrono::milliseconds(100)));
    
    scheduler_->run();
    
    EXPECT_EQ(scheduler_->getCompletedTaskCount(), 2);
}

TEST_F(SchedulerTest, Reset) {
    scheduler_->addTask(std::make_unique<scheduler::Task>(1, 1, std::chrono::milliseconds(100)));
    scheduler_->run();
    
    EXPECT_EQ(scheduler_->getCompletedTaskCount(), 1);
    
    scheduler_->reset();
    EXPECT_EQ(scheduler_->getCompletedTaskCount(), 0);
}