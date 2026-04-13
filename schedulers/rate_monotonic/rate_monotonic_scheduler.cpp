#include <iostream>
#include <algorithm>
#include <numeric>

#include "rate_monotonic_scheduler.hpp"

RM_scheduler::RM_scheduler(unsigned int tx_power, unsigned int frequency, BufferPacket* buffer, std::shared_ptr<unsigned int> sys_tick): BaseScheduler(buffer, sys_tick), tx_power(tx_power), frequency(frequency) {};

scheduled_frame_t RM_scheduler::do_schedule_frame(void)
{
    scheduled_frame_t scheduled_frame;
    scheduled_frame.transmission_power = this->tx_power;
    scheduled_frame.frequency = this->frequency;

    /* Find the packet with the smaller period */
    auto lowest_it = std::min_element(this->buffer_packet->begin(),
                                      this->buffer_packet->end(),
                                      [](const packet_t& a, const packet_t& b) {
                                          if (!a.is_periodic) return false;
                                          if (!b.is_periodic) return true;
                                          return a.period < b.period;
                                      });

    if (lowest_it != this->buffer_packet->end() && lowest_it->is_periodic)
    {
        scheduled_frame.packet = &(*lowest_it);
        scheduled_frame.radio_mode = TX_MODE;
    }
    else
    {
        scheduled_frame.packet = nullptr;
        scheduled_frame.radio_mode = IDLE;
    }

    return scheduled_frame;
}

std::string RM_scheduler::get_name() const {
    return "Rate_M";
}
