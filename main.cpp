#include <iostream>
#include <optional>
#include <vector>
#include <memory>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

/* System Model */
#include "system_model/system_model.hpp"
#include "system_model/buffer_packet/buffer_packet.hpp"
#include "system_model/radio_interface/radio_interface.hpp"
#include "system_model/target_receiver/target_receiver.hpp"
#include "system_model/ml_predictor/ml_predictor.hpp"

#include "packet_generators/fixed_rate/fixed_rate.hpp"
#include "schedulers/earliest_deadline_first/edf_scheduler.hpp"
#include "physical_channels/sigmoid_channel/sigmoid_channel.hpp"

int main()
{
    std::shared_ptr<unsigned int> system_tick = std::make_shared<unsigned int>(0U);

    /* Initialize packet buffer */
    BufferPacket buffer(system_tick);
    std::vector<fixed_rate_packet_t> fixed_rate_packets;

    /* Initialize packet generators */
    fixed_rate_packets.push_back(fixed_rate_packet_t(5U, 2U, 0.9, 1U, 5U, 0U));
    fixed_rate_packets.push_back(fixed_rate_packet_t(4U, 1U, 0.7, 2U, 4U, 0U));

    /* Shared spawn log for all generators */
    std::shared_ptr<json> spawn_log = std::make_shared<json>(json::array());

    FixedRate_PacketGen fixed_rate_packet_gen(system_tick, fixed_rate_packets, buffer.buffer_packet, spawn_log);

    /* Initialize scheduler */
    EDF_scheduler scheduler(12, 14074000, buffer.buffer_packet, system_tick);
    scheduled_frame_t scheduled_frame;

    /* Initialize the radio_interface */
    RadioInterface radio_interface(buffer.buffer_packet);
    transmitted_frame_t transmitted_frame;

    /* Initialize the physical channels */
    std::shared_ptr<std::vector<SigmoidChannel>> channels = std::make_shared<std::vector<SigmoidChannel>>();
    channels->emplace_back(14074000);  /* Uses defaults: snr50=10.0, s=2.0, noise=-90.0, pathloss=100 */

    /* Initalize the ML Predictor */
    MLPredictor ml_predictor(system_tick, channels);
    double pred_dec_prob; /* Predicted Decoding probability */

    /* Initialize the target receiver */
    TargetReceiver target_receiver(system_tick);
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

        switch (scheduled_frame.radio_mode)
        {
            case TX_MODE: /* Transmission path */
            {
                auto result = radio_interface.transmit_frame(scheduled_frame);

                if (result.has_value())
                {
                    transmitted_frame = result.value();
                }
                else
                {
                    std::cout << "Failed to transmit a frame" << std::endl;

                    return -1;
                }

                std::cout    << "Transmitted frame: "
                             << "(id: "                << transmitted_frame.packet.id
                             << ", id_count: "         << transmitted_frame.packet.id_count
                             << ", deadline: "         << transmitted_frame.packet.deadline
                             << ", frames: "           << transmitted_frame.packet.frames
                             << ", frame_count: "      << transmitted_frame.packet.frame_count
                             << ", success_rate: "     << transmitted_frame.packet.success_rate_req << ") ";
                std::cout    << std::endl << std::endl;

                /* Find the channel for the packet */
                auto it = std::find_if(channels->begin(), channels->end(),
                                        [&transmitted_frame](const SigmoidChannel& ch) {
                                            return ch.frequency == transmitted_frame.frequency;
                                        });

                if (it != channels->end())
                {
                    recv_frame = it->gen_frame_with_probability(transmitted_frame);
                    std::cout    << "Probability for the frame: " << recv_frame.success_prob
                                 << ", tx_power: "                 << recv_frame.transmission_power
                                 << ", freq_prob_success: "        << recv_frame.packet.success_rate_req << ") ";
                    std::cout << std::endl;

                    std::cout << "Frame successfully decoded: "
                              << target_receiver.recv_frame(recv_frame)
                              << std::endl << std::endl;
                }
                else
                {
                    std::cout << "ERROR: No matching channel found" << std::endl;
                }

                break;
            }
            case RX_MODE: /* Reception path */
                pred_dec_prob = ml_predictor.predict_channel_conditions(scheduled_frame.frequency, scheduled_frame.transmission_power);

                scheduler.receive_prediction(pred_dec_prob);

                break;
            case IDLE:
                /* Do nothing on this iteration */

                break;
            default:

            break;
        }

        buffer.check_deadlines();
        (*system_tick)++;
    }

    BasePacketGenerator::save_to_file(spawn_log, "generated_packets.json");
    target_receiver.save_to_file("received_packets.json");
    scheduler.save_to_file();
}

