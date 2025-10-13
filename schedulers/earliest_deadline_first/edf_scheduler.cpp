#include <iostream>
#include <algorithm>
#include <numeric>

#include "edf_scheduler.hpp"

EDF_scheduler::EDF_scheduler(unsigned int tx_power): tx_power(tx_power) {};

schedule_result_t EDF_scheduler::schedule_packets(system_model_t system_model)
{
   edf_packets.clear();
    for (const auto& packet : *this->priv_queue_packet) {
        edf_packets.emplace_back(packet);
    }
    schedule_result_t schedule_result;
    schedule_result.is_feasible = true;

    BaseScheduler::reset_scheduler();

    while (this->time_frame_counter < system_model.number_of_frames)
    {
        /* Check if there is any missed deadline */
        for (auto& packet : edf_packets)
        {
            /* Check if packet was scheduled */
            if (packet.is_available)
            {
                /* Check if the packet missed its deadline */
                if (((packet.deadline() + packet.comp_cost()) < this->time_frame_counter))
                {
                    std::cout << "Missed packet ID: " << packet.packet_id() << std::endl;
                    std::cout << "Missed packet deadline: " << packet.deadline() << std::endl;
                    std::cout << "Current time_frame: " << this->time_frame_counter << std::endl;

                    /* Updating deadline and recording packet*/
                    schedule_result.missed_deadlines.push_back(packet.original_packet);  // Push original packet
                    packet.deadline() += packet.period;

                    schedule_result.is_feasible = false;
                }
            }
            else /* Check if the unavailable (i.e. already scheduled) packet has arrived again */
            {
                if (packet.deadline() - packet.period <= this->time_frame_counter)
                {
                    packet.is_available = true;
                }
            }
        }

        edf_packet_t* lowest_packet = nullptr;

        for (auto& packet : edf_packets) {
            if (packet.is_available) {
                if (lowest_packet == nullptr || packet.deadline() < lowest_packet->deadline()) {
                    lowest_packet = &packet;
                }
            }
        }

        if (lowest_packet == nullptr) {
            /* No packet is available, run idle */
            this->time_frame_counter++;

            schedule_result.frame_allocation.insert(schedule_result.frame_allocation.end(), 1, {0, 0, 0, 0}); /* Schedule idle packet with comp_cost=1, packet_id=0, ap and tx_power=0 */

            continue; /* Go to iteration of the while */

        }

        /* Execute the packet transmission */
        lowest_packet->is_available = false;  // Add semicolon
        lowest_packet->packet_count++;
        this->time_frame_counter += lowest_packet->comp_cost();

        for (unsigned int frame_id = 1; frame_id <= lowest_packet->comp_cost(); ++frame_id)
        {
            schedule_result.frame_allocation.push_back({
            lowest_packet->packet_id(),      /* packet_id */
            frame_id,                        /* packet_frame_id (1 to comp_cost) */
            lowest_packet->packet_count,     /* packet_count */
            this->tx_power                   /* tx_power */
            });
        }

        /* Update the deadline */
        lowest_packet->deadline() += lowest_packet->period;
    }

    return schedule_result;
}

//const edf_packet_t EDF_scheduler::idle_task= edf_packet_t({0, UINT_MAX, 1, 100});
//
std::string EDF_scheduler::get_name() const {
    return "EDF";
}

//void EDF_scheduler::sort_packets_by_deadline()
//{
//    if (priv_queue_packet->empty()) {
//        return {};
//    }
//
//    /* Create a copy to avoid modifying original */
//    //std::vector<packet_t> sorted_packets = *priv_queue_packet;
//
//    /* Sort by deadline (earliest first) */
//    std::sort(priv_queue_packet.begin(), priv_queue_packet.end(),
//        [](const packet_t& a, const packet_t& b) {
//            return a.deadline < b.deadline;
//        });
//}

