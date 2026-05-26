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

private:
    json frame_log;  /* Stores all received packets */

    std::default_random_engine generator;
};
