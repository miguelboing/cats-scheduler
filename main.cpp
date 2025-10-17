#include <iostream>
#include <vector>
#include <memory>

#include "system_model/system_model.hpp"
#include "schedulers/earliest_deadline_first/edf_scheduler.hpp"
#include "system_model/physical_channel/physical_channel.hpp"
#include "system_model/receiver/receiver.hpp"

int main()
{
    EDF_scheduler edf_sch(10, 14074); /* Transmit everything with 10W and at 14.074 MHz*/

    Receiver receiver;

    std::vector<unsigned int> power_levels = {1, 10, 25};
    std::vector<unsigned int> frequencies = {14074, 18100, 7074, 10136};

    system_model_t system_model(20, frequencies, power_levels); /* 20 frames, 3 channels */

    PHYChannel phy_channel(system_model, NORMAL);

    std::vector<packet_t> packets =
    {
     {1, 5, 2, 90},  /* packet_id=1, deadline=5, comp_cost=2, success_rate=90 */
     {2, 4, 1, 85},  /* packet_id=2, deadline=4, comp_cost=1, success_rate=85 */
    };

    auto packet_queue = std::make_shared<std::vector<packet_t>>(packets);

    edf_sch.add_queue(packet_queue);

    schedule_result_t result = edf_sch.schedule_packets(system_model);

    std::cout << "Frame allocation: [";
    bool first = true;
    for (const auto& frame : result.frame_allocation) {
        if (!first) std::cout << ", ";
        std::cout << "(" << frame.packet_id << ", " << frame.packet_count << ", " << frame.packet_frame_id << ", " << frame.tx_power << ")";
        first = false;
    }
    std::cout << "]" << std::endl;

    std::cout << "Channel probabilities: [";
    for (size_t ch = 0; ch < system_model.channels->size(); ++ch) {
        std::cout << "Channel " << ch << " (freq: "
                  << (*system_model.channels)[ch].frequency << "):" << std::endl;

        auto& channel = (*system_model.channels)[ch];
        for (size_t pwr = 0; pwr < channel.tx_power_levels.size(); ++pwr) {
            std::cout << "  Power level " << pwr << ": [";
            bool first = true;
            for (unsigned int frame = 0; frame < system_model.number_of_frames; ++frame) {
                if (!first) std::cout << ", ";
                std::cout << (*channel.channel_condition)[pwr][frame];
                first = false;
            }
            std::cout << "]" << std::endl;
        }
    }

    receiver_result_t receiver_result = receiver.recv_packets(system_model, result);

    std::cout << "Received Frames: [";
    first = true;
    for (const auto& frame: receiver_result.received_frames) {
        if (!first) std::cout << ", ";
        std::cout << frame;
        first = false;
    }
    std::cout << "]" << std::endl;

    return 0;
}

