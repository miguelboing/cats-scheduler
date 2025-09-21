#include "schedulers/earliest_deadline_first/edf_scheduler.hpp"

class EDF_scheduler: public BaseScheduler {
public:
	void addTask(std::shared_ptr<Task> task) {
		task_queue.push_back(task)
	}

	std::vector<std::shared_ptr<Task>> ScheduleTasks() {
		// Add your scheduling logic here
		// scheduled_tasks.push_back(some_task);
		return scheduled_tasks;
	}

	std::string getName() const {
		return "EDF"
	}
};


