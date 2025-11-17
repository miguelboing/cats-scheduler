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

    fixed_rate_packets.push_back(fixed_rate_packet_t(5U, 2U, 90U, 1U, 5U, 0U));
    fixed_rate_packets.push_back(fixed_rate_packet_t(4U, 1U, 70U, 2U, 4U, 0U));

    FixedRate_PacketGen fixed_rate_packet_gen(system_tick, fixed_rate_packets, buffer.buffer_packet);

    /* Initialize scheduler */
    EDF_scheduler scheduler(7, 14074000, buffer.buffer_packet);
    scheduled_packet_t scheduled_packet;

    /* Initialize the transmitter */
    Transmitter transmitter(buffer.buffer_packet);
    transmitted_packet_t transmitted_packet;

    /* Initialize the physical channels */
    std::vector<SigmoidChannel> channels;
    channels.emplace_back(14074000);  /* Uses defaults: snr50=10.0, s=2.0, noise=-90.0, pathloss=100 */

    /* Initialize the receiver */
    Receiver receiver;
    received_packet_t recv_packet;

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

        scheduled_packet = scheduler.schedule_packet();

        transmitted_packet = transmitter.transmit_frame(scheduled_packet);

        std::cout    << "Transmitted packet: "
                     << "(id: "                << transmitted_packet.packet.id
                     << ", id_count: "         << transmitted_packet.packet.id_count
                     << ", deadline: "         << transmitted_packet.packet.deadline
                     << ", frames: "           << transmitted_packet.packet.frames
                     << ", frame_count: "      << transmitted_packet.packet.frame_count
                     << ", success_rate: "     << transmitted_packet.packet.success_rate_req << ") ";
        std::cout    << std::endl << std::endl;

        /* Find the channel for the packet */
        auto it = std::find_if(channels.begin(), channels.end(),
                                [&transmitted_packet](const SigmoidChannel& ch) {
                                    return ch.frequency == transmitted_packet.frequency;
                                });

        if (it != channels.end())
        {
            recv_packet = it->gen_frame_with_probability(transmitted_packet);
            std::cout    << "Probability for the packet: " << recv_packet.success_prob
                         << ", tx_power: "                 << recv_packet.transmission_power
                         << ", freq_prob_success: "        << recv_packet.packet.success_rate_req << ") ";
            std::cout << std::endl << std::endl;
        }
        else
        {
            std::cout << "No matching channel found" << std::endl;
        }

        std::cout << "Packet successfully decoded: "
                  << receiver.recv_packet(recv_packet)
                  << std::endl;

        buffer.check_deadlines();
        (*system_tick)++;
    }
}

