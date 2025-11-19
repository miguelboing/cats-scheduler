#include <iostream>
#include <vector>
#include <memory>

/* System Model */
#include "system_model/system_model.hpp"
#include "system_model/buffer_packet/buffer_packet.hpp"
#include "system_model/transmitter/transmitter.hpp"
#include "system_model/receiver/receiver.hpp"

#include "packet_generators/fixed_rate/fixed_rate.hpp"
#include "schedulers/earliest_deadline_first/edf_scheduler.hpp"
#include "physical_channels/sigmoid_channel/sigmoid_channel.hpp"

int main()
{
    std::shared_ptr<unsigned int> system_tick = std::make_shared<unsigned int>(0U);

    /* Initialize packet buffer */
    BufferPacket buffer(system_tick);
    std::vector<fixed_rate_packet_t> fixed_rate_packets;

    fixed_rate_packets.push_back(fixed_rate_packet_t(5U, 2U, 0.9, 1U, 5U, 0U));
    fixed_rate_packets.push_back(fixed_rate_packet_t(4U, 1U, 0.7, 2U, 4U, 0U));

    FixedRate_PacketGen fixed_rate_packet_gen(system_tick, fixed_rate_packets, buffer.buffer_packet);

    /* Initialize scheduler */
    EDF_scheduler scheduler(12, 14074000, buffer.buffer_packet);
    scheduled_frame_t scheduled_frame;

    /* Initialize the transmitter */
    Transmitter transmitter(buffer.buffer_packet);
    transmitted_frame_t transmitted_frame;

    /* Initialize the physical channels */
    std::vector<SigmoidChannel> channels;
    channels.emplace_back(14074000);  /* Uses defaults: snr50=10.0, s=2.0, noise=-90.0, pathloss=100 */

    /* Initialize the receiver */
    Receiver receiver(system_tick);
    received_frame_t recv_frame;

    for (unsigned int i = 0U; i < 20U; i++)
    {
        fixed_rate_packet_gen.generate_packets();

        std::cout << "Frame " << *system_tick << " - Buffer contents: " << std::endl;
        for (const auto& packet : *buffer.buffer_packet) {
            std::cout << "(id: "            << packet.id
                      << ", id_count: "     << packet.id_count
                      << ", deadline: "     << packet.deadline
                      << ", frames: "       << packet.frames
                      << ", success_rate: " << packet.success_rate_req << ") ";
            std::cout << std::endl;
        }

        std::cout << std::endl;

        scheduled_frame = scheduler.schedule_frame();

        transmitted_frame = transmitter.transmit_frame(scheduled_frame);

        std::cout    << "Transmitted packet: "
                     << "(id: "                << transmitted_frame.packet.id
                     << ", id_count: "         << transmitted_frame.packet.id_count
                     << ", deadline: "         << transmitted_frame.packet.deadline
                     << ", frames: "           << transmitted_frame.packet.frames
                     << ", frame_count: "      << transmitted_frame.packet.frame_count
                     << ", success_rate: "     << transmitted_frame.packet.success_rate_req << ") ";
        std::cout    << std::endl << std::endl;

        /* Find the channel for the packet */
        auto it = std::find_if(channels.begin(), channels.end(),
                                [&transmitted_frame](const SigmoidChannel& ch) {
                                    return ch.frequency == transmitted_frame.frequency;
                                });

        if (it != channels.end())
        {
            recv_frame = it->gen_frame_with_probability(transmitted_frame);
            std::cout    << "Probability for the frame: " << recv_frame.success_prob
                         << ", tx_power: "                 << recv_frame.transmission_power
                         << ", freq_prob_success: "        << recv_frame.packet.success_rate_req << ") ";
            std::cout << std::endl;

            std::cout << "Frame successfully decoded: "
                      << receiver.recv_frame(recv_frame)
                      << std::endl << std::endl;
        }
        else
        {
            std::cout << "ERROR: No matching channel found" << std::endl;
        }

        buffer.check_deadlines();
        (*system_tick)++;
    }

    receiver.save_to_file("receiver_results.json");
}

