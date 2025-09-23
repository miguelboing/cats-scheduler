#pragma once

#include <memory>

#include "schedulers/base_scheduler.hpp"

class EDF_scheduler : public BaseScheduler {
public:
    schedule_result_t schedule_packets(system_model_t system_model) override;

    std::string get_name() const override;
private:
 std::vector<packet_t> sort_packets_by_deadline();
};





