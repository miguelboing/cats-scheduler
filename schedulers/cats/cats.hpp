#pragma once

#include <memory>

#include "schedulers/base_scheduler.hpp"
#include <unordered_map>

class CATS_scheduler : public BaseScheduler
{
public:
    CATS_scheduler(unsigned int frequency, unsigned int rx_period, BufferPacket* buffer, std::shared_ptr<unsigned int> sys_tick);

    unsigned int frequency;
    unsigned int rx_period;
    double transmission_prob[3];
    unsigned int retransmissions_per_frame;
    std::unordered_map<unsigned int, double> accumulated_prob;

    scheduled_frame_t do_schedule_frame(void) override;

    void receive_prediction(const std::vector<double>& pred_probs) override;

    std::vector<unsigned int> get_prediction_powers() const override { return {1, 10, 25}; }

    std::string get_name() const override;
};

