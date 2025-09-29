#include <iostream>
#include <algorithm>
#include <numeric>

#include "edf_scheduler.hpp"

schedule_result_t EDF_scheduler::schedule_packets(system_model_t system_model)
{
    schedule_result_t schedule_result;
    schedule_result.is_feasible = true;

    BaseScheduler::reset_scheduler();

    while (this->time_frame_counter < system_model.number_of_frames)
    {
        /* Check if there is any missed deadline */
        for (auto& packet: *this->priv_queue_packet)
        {
            /* Check for missed deadline */
            if ((packet.deadline + packet.comp_cost) < this->time_frame_counter)
            {
                std::cout << "Missed packet ID: " << packet.packet_id << std::endl;
                std::cout << "Missed packet deadline: " << packet.deadline << std::endl;
                std::cout << "Current time_frame: " << this->time_frame_counter << std::endl;

                /* Updating deadline and recording packet*/
                schedule_result.missed_deadlines.push_back(packet);
                packet.deadline += packet.deadline;

                schedule_result.is_feasible = false;
            }
        }

        /* Find the packet with the lowest period */
        auto lowest_it = std::min_element(priv_queue_packet->begin(), priv_queue_packet->end(),
          [](const packet_t& a, const packet_t& b) {
              return a.deadline < b.deadline;
          });

        /* Execute the packet transmission */
        this->time_frame_counter += lowest_it->comp_cost;
        schedule_result.frame_allocation.insert(schedule_result.frame_allocation.end(), lowest_it->comp_cost, lowest_it->packet_id);  /* Append comp_cost copies of task_id */

        /* Update the deadline */
        lowest_it->deadline += lowest_it->deadline;
    }

    return schedule_result;
}

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

