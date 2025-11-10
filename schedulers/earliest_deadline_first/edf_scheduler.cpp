#include <iostream>
#include <algorithm>
#include <numeric>

#include "edf_scheduler.hpp"

EDF_scheduler::EDF_scheduler(unsigned int tx_power, unsigned int frequency, std::shared_ptr<std::vector<packet_t>> buffer_packet): BaseScheduler(buffer_packet), tx_power(tx_power), frequency(frequency) {};

scheduled_packet_t EDF_scheduler::schedule_packet(void)
{
    scheduled_packet_t scheduled_packet;
    scheduled_packet.transmission_power = this->tx_power;
    scheduled_packet.frequency = this->frequency;

    /* Find the packet with the earliest deadline */
    auto lowest_it = std::min_element(this->buffer_packet->begin(),
                                      this->buffer_packet->end(),
                                      [](const packet_t& a, const packet_t& b) {
                                          return a.deadline < b.deadline;
                                      });
    if (lowest_it != this->buffer_packet->end())
    {
        scheduled_packet.packet = &(*lowest_it);
    }
    else
    {
        scheduled_packet.packet = nullptr; /* Means idle/no tranmission */
    }


    return scheduled_packet;
}

std::string EDF_scheduler::get_name() const {
    return "EDF";
}

