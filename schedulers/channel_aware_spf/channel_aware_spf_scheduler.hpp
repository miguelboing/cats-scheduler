#pragma once

#include <memory>

#include "schedulers/base_scheduler.hpp"
#include <unordered_map>

class CHASPF_scheduler : public BaseScheduler
{
public:
    CHASPF_scheduler(unsigned int tx_power, unsigned int frequency, unsigned int rx_period, std::shared_ptr<std::vector<packet_t>> buffer_packet, std::shared_ptr<unsigned int> sys_tick);

    unsigned int tx_power;
    unsigned int frequency;
    unsigned int rx_period;
    double transmission_prob;
    std::unordered_map<unsigned int, double> accumulated_prob;

    scheduled_frame_t do_schedule_frame(void) override;

    void receive_prediction(double pred_dec_prob) override;

    std::string get_name() const override;
};

