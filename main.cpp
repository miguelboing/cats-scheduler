#include <iostream>
#include <fstream>
#include <optional>
#include <vector>
#include <memory>
#include <map>
#include <set>
#include <tuple>
#include <utility>
#include <algorithm>
#include <streambuf>
#include <cstdint>

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

// Null streambuf used to silence std::cout cheaply when running in summary
// mode — keeps the hot loop free of pipe writes and flushes.
struct NullBuf : std::streambuf {
    int overflow(int c) override { return c; }
};

int main(int argc, char* argv[])
{
    const std::string config_file = (argc > 1) ? argv[1] : "simulation_config.json";
    const std::string log_file    = (argc > 2) ? argv[2] : "simulation_log.json";
    // argv[3] == "summary" activates summary-only mode: stdout silenced,
    // per-frame JSON log skipped, and a tiny aggregate written to log_file.
    const bool summary_only       = (argc > 3) && std::string(argv[3]) == "summary";

    NullBuf null_buf;
    std::streambuf* orig_cout = nullptr;
    if (summary_only) orig_cout = std::cout.rdbuf(&null_buf);

    std::ifstream f(config_file);
    if (!f.is_open())
    {
        std::cerr << "ERROR: Could not open config file: " << config_file << std::endl;
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
            if (summary_only) packet_gens.back().set_log_enabled(false);
        }
    }

    // Optional master seed for reproducibility. When present, every RNG is
    // reseeded with a distinct offset so streams stay independent.
    const bool has_seed   = config["simulation"].contains("seed");
    const uint64_t seed   = has_seed ? config["simulation"]["seed"].get<uint64_t>() : 0;

    /* Initialize channels from config */
    std::shared_ptr<std::vector<SigmoidChannel>> channels = std::make_shared<std::vector<SigmoidChannel>>();
    {
        size_t ch_idx = 0;
        for (const auto& ch : config["channels"])
        {
            if (ch["type"] == "sigmoid")
            {
                channels->emplace_back(ch["frequency"], ch["name"]);
                if (has_seed)
                    channels->back().seed_rng(seed + 0x100 + ch_idx);
            }
            ++ch_idx;
        }
    }

    /* Initialize scheduler from config */
    const auto& sched_cfg = config["scheduler"];
    const std::string sched_type = sched_cfg["type"];

    std::unique_ptr<BaseScheduler> scheduler;
    if (sched_type == "CHARM")
    {
        scheduler = std::make_unique<CHARM_scheduler>(
            sched_cfg["tx_power"],
            sched_cfg["frequency"],
            sched_cfg["rx_period"],
            &buffer,
            system_tick
        );
    }
    else if (sched_type == "EDF")
    {
        scheduler = std::make_unique<EDF_scheduler>(
            sched_cfg["tx_power"],
            sched_cfg["frequency"],
            &buffer,
            system_tick
        );
    }
    else if (sched_type == "Rate_M")
    {
        scheduler = std::make_unique<RM_scheduler>(
            sched_cfg["tx_power"],
            sched_cfg["frequency"],
            &buffer,
            system_tick
        );
    }
    else if (sched_type == "CATS")
    {
        scheduler = std::make_unique<CATS_scheduler>(
            sched_cfg["frequency"],
            sched_cfg["belief_threshold"],
            sched_cfg["margin"],
            &buffer,
            system_tick
        );
    }
    else
    {
        std::cerr << "ERROR: Unknown scheduler type: " << sched_type << std::endl;
        return -1;
    }

    if (summary_only) scheduler->set_log_enabled(false);

    scheduled_frame_t scheduled_frame;

    /* Initialize the radio_interface */
    RadioInterface radio_interface(buffer.buffer_packet);
    transmitted_frame_t transmitted_frame;

    /* Initialize the ML Predictor */
    MLPredictor ml_predictor(system_tick, channels);
    std::vector<double> pred_probs;

    /* Initialize the target receiver */
    TargetReceiver target_receiver(system_tick);
    if (has_seed) target_receiver.seed_rng(seed + 0x200);
    if (summary_only) target_receiver.set_log_enabled(false);
    received_frame_t recv_frame;

    const unsigned int duration = config["simulation"]["duration"];
    json simulation_log = json::array();

    // Summary-only aggregates — only touched when summary_only is true.
    std::map<std::pair<int,int>, int> instance_frames_needed;
    std::set<std::tuple<int,int,int>> received_slots;
    double total_tx_power = 0.0;

    for (unsigned int i = 0U; i < duration; i++)
    {
        for (auto& gen : packet_gens)
            gen.generate_packets();

        json frame_entry;
        if (!summary_only)
        {
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
        }

        scheduled_frame = scheduler->schedule_frame();

        if (!summary_only) std::cout << "Radio is in ";

        switch (scheduled_frame.radio_mode)
        {
            case TX_MODE:
            {
                if (!summary_only)
                {
                    std::cout << "TX MODE" << std::endl;
                    frame_entry["radio_mode"] = "TX_MODE";
                }

                auto result = radio_interface.transmit_frame(scheduled_frame);

                if (result.has_value())
                {
                    transmitted_frame = result.value();
                }
                else
                {
                    std::cerr << "Failed to transmit a frame" << std::endl;
                    return -1;
                }

                if (!summary_only)
                {
                    std::cout    << "Transmitted frame: "
                                 << "(id: "                << transmitted_frame.packet.id
                                 << ", id_count: "         << transmitted_frame.packet.id_count
                                 << ", deadline: "         << transmitted_frame.packet.deadline
                                 << ", frames: "           << transmitted_frame.packet.frames
                                 << ", frame_count: "      << transmitted_frame.packet.frame_count
                                 << ", success_rate: "     << transmitted_frame.packet.success_rate_req << ") ";
                    std::cout    << std::endl;
                }

                auto it = std::find_if(channels->begin(), channels->end(),
                                        [&transmitted_frame](const SigmoidChannel& ch) {
                                            return ch.frequency == transmitted_frame.frequency;
                                        });

                if (it != channels->end())
                {
                    recv_frame = it->gen_frame_with_probability(transmitted_frame);
                    bool received = target_receiver.recv_frame(recv_frame);

                    if (summary_only)
                    {
                        const int pid       = transmitted_frame.packet.id;
                        const int pid_count = transmitted_frame.packet.id_count;
                        instance_frames_needed[{pid, pid_count}] = transmitted_frame.packet.frames;
                        total_tx_power += recv_frame.transmission_power;
                        if (received)
                        {
                            received_slots.insert(std::make_tuple(
                                pid, pid_count, transmitted_frame.packet.frame_count));
                        }
                    }
                    else
                    {
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
                }
                else
                {
                    std::cerr << "ERROR: No matching channel found" << std::endl;
                }

                break;
            }
            case RX_MODE:
                if (!summary_only)
                {
                    std::cout << "RX MODE" << std::endl;
                    frame_entry["radio_mode"] = "RX_MODE";
                }

                pred_probs = ml_predictor.predict_channel_conditions(scheduled_frame.frequency, scheduler->get_prediction_powers());

                if (!summary_only)
                {
                    std::cout << "Predicted probabilities at frequency " << scheduled_frame.frequency << "Hz:";
                    for (size_t k = 0; k < scheduler->get_prediction_powers().size(); k++)
                        std::cout << " " << scheduler->get_prediction_powers()[k] << "W=" << pred_probs[k];
                    std::cout << std::endl;
                }

                scheduler->receive_prediction(pred_probs);

                if (!summary_only)
                {
                    frame_entry["prediction"] = {
                        {"powers",      scheduler->get_prediction_powers()},
                        {"probs",       pred_probs},
                        {"frequency",   scheduled_frame.frequency}
                    };
                }

                break;

            case IDLE:
                if (!summary_only)
                {
                    std::cout << "IDLE MODE" << std::endl;
                    frame_entry["radio_mode"] = "IDLE";
                }
                break;

            default:
                break;
        }

        /* Log missed packets */
        auto missed_packets = buffer.check_deadlines();
        if (!summary_only) frame_entry["missed_packets"] = json::array();
        for (const auto& missed : missed_packets)
        {
            if (summary_only)
            {
                auto key = std::make_pair(missed.id, missed.id_count);
                if (instance_frames_needed.find(key) == instance_frames_needed.end())
                    instance_frames_needed[key] = missed.frames;
            }
            else
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
                    {"frame_count",      missed.frame_count},
                    {"success_rate_req", missed.success_rate_req}
                });
            }
        }

        /* Log dropped packets */
        if (!summary_only) frame_entry["dropped_packets"] = json::array();
        for (const auto& dropped : buffer.dropped_packets)
        {
            if (summary_only)
            {
                auto key = std::make_pair(dropped.id, dropped.id_count);
                if (instance_frames_needed.find(key) == instance_frames_needed.end())
                    instance_frames_needed[key] = dropped.frames;
            }
            else
            {
                std::cout << "Dropped packet:"
                          << " ID: "       << dropped.id
                          << " ID Count: " << dropped.id_count
                          << " Deadline: " << dropped.deadline
                          << " Frames: "   << dropped.frames
                          << " SR Req: "   << dropped.success_rate_req
                          << std::endl;

                frame_entry["dropped_packets"].push_back({
                    {"id",               dropped.id},
                    {"id_count",         dropped.id_count},
                    {"deadline",         dropped.deadline},
                    {"frames",           dropped.frames},
                    {"frame_count",      dropped.frame_count},
                    {"success_rate_req", dropped.success_rate_req}
                });
            }
        }
        buffer.dropped_packets.clear();

        /* Log FSMC status */
        if (!summary_only)
        {
            std::cout << "\nFSMC STATUS" << std::endl;
            frame_entry["fsmc"] = json::array();
        }
        for (auto& ch : *channels)
        {
            int fsmc_state = ch.get_fsmc_state();
            if (!summary_only)
            {
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
            }

            ch.advance_fsmc_state();
        }

        if (!summary_only) simulation_log.push_back(frame_entry);
        (*system_tick)++;
    }

    if (orig_cout) std::cout.rdbuf(orig_cout);

    if (summary_only)
    {
        // Aggregate per-id instance counts. Mirrors the Python extract_metrics
        // logic so sched_ratio matches the full-log path exactly.
        std::map<int, int> per_id_generated;
        std::map<int, int> per_id_undelivered;
        std::map<int, double> per_id_req;
        for (const auto& gen : config["packet_generators"])
        {
            for (const auto& p : gen["packets"])
            {
                int pid = p["id"];
                per_id_req[pid]         = p["success_rate"];
                per_id_generated[pid]   = 0;
                per_id_undelivered[pid] = 0;
            }
        }
        for (const auto& kv : instance_frames_needed)
        {
            const int pid       = kv.first.first;
            const int pid_count = kv.first.second;
            const int needed    = kv.second;
            per_id_generated[pid]++;
            int delivered = 0;
            for (int slot = 0; slot < needed; slot++)
            {
                if (received_slots.count(std::make_tuple(pid, pid_count, slot)))
                    delivered++;
            }
            if (delivered < needed) per_id_undelivered[pid]++;
        }

        json summary;
        summary["total_tx_power"] = total_tx_power;
        json per_id = json::array();
        for (const auto& kv : per_id_req)
        {
            const int pid = kv.first;
            per_id.push_back({
                {"id",                pid},
                {"success_rate_req",  kv.second},
                {"generated",         per_id_generated[pid]},
                {"undelivered",       per_id_undelivered[pid]}
            });
        }
        summary["per_id"] = per_id;

        std::ofstream sim_log_file(log_file);
        sim_log_file << summary.dump();
    }
    else
    {
        /* Save simulation log */
        std::ofstream sim_log_file(log_file);
        sim_log_file << simulation_log.dump(4);

        BasePacketGenerator::save_to_file(spawn_log, "generated_packets.json");
        target_receiver.save_to_file("received_packets.json");
        scheduler->save_to_file();
    }
}
