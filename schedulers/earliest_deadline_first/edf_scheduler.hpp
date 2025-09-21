#pragma once
#include "schedulers/base_scheduler.hpp"

class EDF_scheduler : public BaseScheduler {
public:
    void addTask(std::shared_ptr<Task> task) override;

    std::vector<std::shared_ptr<Task>> ScheduleTasks() override;

    std::string getName() const override;

};

