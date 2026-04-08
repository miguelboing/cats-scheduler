#pragma once

#include <memory>

#include "schedulers/base_scheduler.hpp"
#include <unordered_map>

class CHASPF_scheduler : public BaseScheduler
{
public:
    CHASPF_scheduler(unsigned int tx_power, unsigned int frequency, unsigned int rx_period, BufferPacket* buffer, std::shared_ptr<unsigned int> sys_tick);

    unsigned int tx_power;
    unsigned int frequency;
    unsigned int rx_period;
    double transmission_prob;
    std::unordered_map<unsigned int, double> accumulated_prob;

    scheduled_frame_t do_schedule_frame(void) override;

    void receive_prediction(const std::vector<double>& pred_probs) override;
    std::vector<unsigned int> get_prediction_powers() const override { return {10}; }

    std::string get_name() const override;
};

