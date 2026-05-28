#pragma once

#include <cstdint>
#include <random>
#include <vector>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "system_model/system_model.hpp"

class TargetReceiver
{
public:
    explicit TargetReceiver(std::shared_ptr<unsigned int> sys_tick);

    bool recv_frame(received_frame_t recv_frame);

    std::shared_ptr<unsigned int> system_tick;

    void save_to_file(const std::string& filename);

    // Reseed the bernoulli draw RNG for deterministic replication.
    void seed_rng(uint64_t seed);

    /* Toggle per-frame logging. Disable in summary mode so the per-binary
       memory footprint stays flat across long-duration runs. */
    void set_log_enabled(bool enabled) { log_enabled = enabled; }

private:
    json frame_log;  /* Stores all received packets */
    bool log_enabled = true;

    std::default_random_engine generator;
};
