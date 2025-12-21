#pragma once

#include <memory>

#include "schedulers/base_scheduler.hpp"

class EDF_scheduler : public BaseScheduler
{
public:
    EDF_scheduler(unsigned int tx_power, unsigned int frequency, std::shared_ptr<std::vector<packet_t>> buffer_packet);
    unsigned int tx_power;
    unsigned int frequency;
    scheduled_frame_t schedule_frame(void) override;

    std::string get_name() const override;
};





