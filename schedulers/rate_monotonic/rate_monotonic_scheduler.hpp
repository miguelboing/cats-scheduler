#pragma once

#include <memory>

#include "schedulers/base_scheduler.hpp"

class RM_scheduler : public BaseScheduler
{
public:
    RM_scheduler(unsigned int tx_power, unsigned int frequency, BufferPacket* buffer, std::shared_ptr<unsigned int> sys_tick);
    unsigned int tx_power;
    unsigned int frequency;
    scheduled_frame_t do_schedule_frame(void) override;

    void receive_prediction(const std::vector<double>& pred_probs) override {}
    std::vector<unsigned int> get_prediction_powers() const override { return {}; }

    std::string get_name() const override;
};
