#pragma once

#include <memory>

#include "schedulers/base_scheduler.hpp"

class EDF_scheduler : public BaseScheduler
{
public:
    EDF_scheduler(unsigned int tx_power, unsigned int frequency, BufferPacket* buffer, std::shared_ptr<unsigned int> sys_tick);
    unsigned int tx_power;
    unsigned int frequency;
    scheduled_frame_t do_schedule_frame(void) override;

    void receive_prediction(const std::vector<double>& pred_probs) override {}
    /* No get_prediction_powers() override: this scheduler transmits at a fixed
       power and needs no predictions, which is the base class's empty default. */

    std::string get_name() const override;
};

