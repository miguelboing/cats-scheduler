#include <iostream>
#include <algorithm>
#include <numeric>

#include "edf_scheduler.hpp"

EDF_scheduler::EDF_scheduler(unsigned int tx_power, unsigned int frequency, std::shared_ptr<std::vector<packet_t>> buffer_packet): BaseScheduler(buffer_packet), tx_power(tx_power), frequency(frequency) {};

packet_t& EDF_scheduler::schedule_packets(void)
{
   auto lowest_it = std::min_element(this->buffer_packet->begin(),
                                     this->buffer_packet->end(),
                                     [](const packet_t& a, const packet_t& b) {
                                         return a.deadline < b.deadline;
                                     });
   return *lowest_it;
}

std::string EDF_scheduler::get_name() const {
    return "EDF";
}

