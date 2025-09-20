#pragma once
#include <vector>
#include <memory>

#include "system_model/packet.hpp"

class Task;

class BaseScheduler {
public:
    virtual ~BaseScheduler() = default;

    /* Pure virtual functions that all schedulers must implement */
    virtual void addTask(std::shared_ptr<Task> task) = 0;
    //virtual std::shared_ptr<Process> getNextProcess() = 0;
    virtual void ScheduleTasks() = 0;
    //virtual bool hasTasks() const = 0;
    virtual std::string getName() const = 0;

//protected:
//    std::vector<std::shared_ptr<Process>> ready_queue;
};

