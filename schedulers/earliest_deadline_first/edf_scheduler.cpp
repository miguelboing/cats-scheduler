#include <algorithm>

#include "schedulers/earliest_deadline_first/edf_scheduler.hpp"

schedule_result_t EDF_scheduler::schedule_packets(system_model_t system_model)
{
    unsigned int time_frame_counter = 0U;
    std::vector<packet_t> scheduled_packets;
    schedule_result_t schedule_result;

    BaseScheduler::reset_scheduler();

    scheduled_packets = EDF_scheduler::sort_packets_by_deadline();

    /* TODO:Add logic to iterate between each packet, and advance with the time_frame_counter */

    return schedule_result;
}

std::string EDF_scheduler::get_name() const {
    return "EDF";
}

std::vector<packet_t> EDF_scheduler::sort_packets_by_deadline()
{
    if (priv_queue_packet->empty()) {
        return {};
    }

    /* Create a copy to avoid modifying original */
    std::vector<packet_t> sorted_packets = *priv_queue_packet;

    /* Sort by deadline (earliest first) */
    std::sort(sorted_packets.begin(), sorted_packets.end(),
        [](const packet_t& a, const packet_t& b) {
            return a.deadline < b.deadline;
        });

        return sorted_packets;
}

