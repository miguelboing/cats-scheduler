#include <iostream>
#include <algorithm>
#include <numeric>

#include "spf_scheduler.hpp"

SPF_scheduler::SPF_scheduler(unsigned int tx_power, unsigned int frequency, std::shared_ptr<std::vector<packet_t>> buffer_packet, std::shared_ptr<unsigned int> sys_tick): BaseScheduler(buffer_packet, sys_tick), tx_power(tx_power), frequency(frequency) {};

scheduled_frame_t SPF_scheduler::do_schedule_frame(void)
{
    scheduled_frame_t scheduled_frame;
    scheduled_frame.transmission_power = this->tx_power;
    scheduled_frame.frequency = this->frequency;

    /* Find the packet with the smaller period */
    auto lowest_it = std::min_element(this->buffer_packet->begin(),
                                      this->buffer_packet->end(),
                                      [](const packet_t& a, const packet_t& b) {
                                          if (!a.is_period) return false;
                                          if (!b.is_period) return true;
                                          return a.period < b.period;
                                      });

    if (lowest_it != this->buffer_packet->end() && lowest_it->is_period)
    {
        scheduled_frame.packet = &(*lowest_it);
        scheduled_frame.radio_mode = TX_MODE;
    }
    else
    {
        scheduled_frame.packet = nullptr; /* Means idle/no tranmission */
        scheduled_frame.radio_mode = IDLE;
    }


    return scheduled_frame;
}

std::string SPF_scheduler::get_name() const {
    return "SPF";
}

