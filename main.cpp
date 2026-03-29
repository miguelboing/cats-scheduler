#include <iostream>
#include <fstream>
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
#include "physical_channels/sigmoid_channel/sigmoid_channel.hpp"

#include "schedulers.hpp"

int main(int argc, char* argv[])
{
    const std::string config_file = (argc > 1) ? argv[1] : "simulation_config.json";
    std::ifstream f(config_file);
    if (!f.is_open())
    {
        std::cout << "ERROR: Could not open config file: " << config_file << std::endl;
        return -1;
    }
    json config = json::parse(f);

    std::shared_ptr<unsigned int> system_tick = std::make_shared<unsigned int>(0U);

    /* Initialize packet buffer */
    BufferPacket buffer(system_tick);

    /* Shared spawn log for all generators */
    std::shared_ptr<json> spawn_log = std::make_shared<json>(json::array());

    /* Initialize packet generators from config */
    std::vector<FixedRate_PacketGen> packet_gens;
    for (const auto& gen : config["packet_generators"])
    {
        if (gen["type"] == "fixed_rate")
        {
            std::vector<fixed_rate_packet_t> packets;
            for (const auto& p : gen["packets"])
            {
                packets.push_back(fixed_rate_packet_t(
                    p["relative_deadline"],
                    p["frames"],
                    p["success_rate"],
                    p["id"],
                    p["period"],
                    p["phase"]
                ));
            }
            packet_gens.emplace_back(system_tick, packets, buffer.buffer_packet, spawn_log);
        }
    }

    /* Initialize channels from config */
    std::shared_ptr<std::vector<SigmoidChannel>> channels = std::make_shared<std::vector<SigmoidChannel>>();
    for (const auto& ch : config["channels"])
    {
        if (ch["type"] == "sigmoid")
            channels->emplace_back(ch["frequency"], ch["name"]);
    }

    /* Initialize scheduler from config */
    const auto& sched_cfg = config["scheduler"];
    const std::string sched_type = sched_cfg["type"];

    std::unique_ptr<BaseScheduler> scheduler;
    if (sched_type == "CHASPF")
    {
        scheduler = std::make_unique<CHASPF_scheduler>(
            sched_cfg["tx_power"],
            sched_cfg["frequency"],
            sched_cfg["rx_period"],
            buffer.buffer_packet,
            system_tick
        );
    }
    else if (sched_type == "EDF")
    {
        scheduler = std::make_unique<EDF_scheduler>(
            sched_cfg["tx_power"],
            sched_cfg["frequency"],
            buffer.buffer_packet,
            system_tick
        );
    }
    else if (sched_type == "SPF")
    {
        scheduler = std::make_unique<SPF_scheduler>(
            sched_cfg["tx_power"],
            sched_cfg["frequency"],
            buffer.buffer_packet,
            system_tick
        );
    }
    else
    {
        std::cout << "ERROR: Unknown scheduler type: " << sched_type << std::endl;
        return -1;
    }

    scheduled_frame_t scheduled_frame;

    /* Initialize the radio_interface */
    RadioInterface radio_interface(buffer.buffer_packet);
    transmitted_frame_t transmitted_frame;

    /* Initialize the ML Predictor */
    MLPredictor ml_predictor(system_tick, channels);
    double pred_dec_prob;

    /* Initialize the target receiver */
    TargetReceiver target_receiver(system_tick);
    received_frame_t recv_frame;

    const unsigned int duration = config["simulation"]["duration"];
    json simulation_log = json::array();

    for (unsigned int i = 0U; i < duration; i++)
    {
        for (auto& gen : packet_gens)
            gen.generate_packets();

        json frame_entry;
        frame_entry["tick"] = *system_tick;

        /* Log buffer state */
        frame_entry["buffer"] = json::array();
        for (const auto& packet : *buffer.buffer_packet)
        {
            frame_entry["buffer"].push_back({
                {"id",               packet.id},
                {"id_count",         packet.id_count},
                {"deadline",         packet.deadline},
                {"frames",           packet.frames},
                {"frame_count",      packet.frame_count},
                {"success_rate_req", packet.success_rate_req}
            });
        }

        std::cout << "--------------------------------------------------------------------------------" << std::endl;
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

        scheduled_frame = scheduler->schedule_frame();

        std::cout << "Radio is in ";

        switch (scheduled_frame.radio_mode)
        {
            case TX_MODE:
            {
                std::cout << "TX MODE" << std::endl;
                frame_entry["radio_mode"] = "TX_MODE";

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
                std::cout    << std::endl;

                auto it = std::find_if(channels->begin(), channels->end(),
                                        [&transmitted_frame](const SigmoidChannel& ch) {
                                            return ch.frequency == transmitted_frame.frequency;
                                        });

                if (it != channels->end())
                {
                    recv_frame = it->gen_frame_with_probability(transmitted_frame);
                    bool received = target_receiver.recv_frame(recv_frame);

                    std::cout   << "TX Power : "                   << recv_frame.transmission_power
                                << "\nProbability for the frame: " << recv_frame.success_prob
                                << " and required probability: "   << recv_frame.packet.success_rate_req;
                    std::cout << std::endl;
                    std::cout << "Frame successfully received by the target? " << received << std::endl;

                    frame_entry["transmission"] = {
                        {"packet", {
                            {"id",               transmitted_frame.packet.id},
                            {"id_count",         transmitted_frame.packet.id_count},
                            {"deadline",         transmitted_frame.packet.deadline},
                            {"frames",           transmitted_frame.packet.frames},
                            {"frame_count",      transmitted_frame.packet.frame_count},
                            {"success_rate_req", transmitted_frame.packet.success_rate_req}
                        }},
                        {"tx_power",         recv_frame.transmission_power},
                        {"frequency",        recv_frame.frequency},
                        {"probability",      recv_frame.success_prob},
                        {"success_rate_req", recv_frame.packet.success_rate_req},
                        {"received",         received}
                    };
                }
                else
                {
                    std::cout << "ERROR: No matching channel found" << std::endl;
                }

                break;
            }
            case RX_MODE:
                std::cout << "RX MODE" << std::endl;
                frame_entry["radio_mode"] = "RX_MODE";

                pred_dec_prob = ml_predictor.predict_channel_conditions(scheduled_frame.frequency, scheduled_frame.transmission_power);

                std::cout << "Predicted probability for "
                          << scheduled_frame.transmission_power << "W at frequency "
                          << scheduled_frame.frequency << "Hz: "
                          << pred_dec_prob;
                std::cout << std::endl;

                scheduler->receive_prediction(pred_dec_prob);

                frame_entry["prediction"] = {
                    {"probability", pred_dec_prob},
                    {"tx_power",    scheduled_frame.transmission_power},
                    {"frequency",   scheduled_frame.frequency}
                };

                break;

            case IDLE:
                std::cout << "IDLE MODE" << std::endl;
                frame_entry["radio_mode"] = "IDLE";
                break;

            default:
                break;
        }

        /* Log missed packets */
        auto missed_packets = buffer.check_deadlines();
        frame_entry["missed_packets"] = json::array();
        for (const auto& missed : missed_packets)
        {
            std::cout << "Missed deadline for packet:"
                      << " ID: "       << missed.id
                      << " ID Count: " << missed.id_count
                      << " Deadline: " << missed.deadline
                      << " Frames: "   << missed.frames
                      << " SR Req: "   << missed.success_rate_req
                      << std::endl;

            frame_entry["missed_packets"].push_back({
                {"id",               missed.id},
                {"id_count",         missed.id_count},
                {"deadline",         missed.deadline},
                {"frames",           missed.frames},
                {"success_rate_req", missed.success_rate_req}
            });
        }

        /* Log FSMC status */
        std::cout << "\nFSMC STATUS" << std::endl;
        frame_entry["fsmc"] = json::array();
        for (auto& ch : *channels)
        {
            int fsmc_state = ch.get_fsmc_state();
            std::cout << "Frequency: " << ch.frequency << std::endl;
            std::cout << "State: " << fsmc_state << std::endl;
            std::cout << "Parameters"
                      << "\nSlope: " << ch.fsmc[fsmc_state].slope
                      << "\nSNR@50% [dB]: " << ch.fsmc[fsmc_state].snr_50_db
                      << "\nMax Saturation: " << ch.fsmc[fsmc_state].max_saturation
                      << "\nNoise Floor [dBm]: " << ch.fsmc[fsmc_state].noise_floor_dbm;
            std::cout << std::endl << std::endl;

            frame_entry["fsmc"].push_back({
                {"frequency",       ch.frequency},
                {"state",           fsmc_state},
                {"slope",           ch.fsmc[fsmc_state].slope},
                {"snr_50_db",       ch.fsmc[fsmc_state].snr_50_db},
                {"max_saturation",  ch.fsmc[fsmc_state].max_saturation},
                {"noise_floor_dbm", ch.fsmc[fsmc_state].noise_floor_dbm}
            });

            ch.advance_fsmc_state();
        }

        simulation_log.push_back(frame_entry);
        (*system_tick)++;
    }

    /* Save simulation log */
    std::ofstream sim_log_file("simulation_log.json");
    sim_log_file << simulation_log.dump(4);

    BasePacketGenerator::save_to_file(spawn_log, "generated_packets.json");
    target_receiver.save_to_file("received_packets.json");
    scheduler->save_to_file();
}
