#pragma once

#include <cstdint>
#include <memory>
#include <random>
#include <vector>

#include "system_model/system_model.hpp"
#include "physical_channels/base_physical_channel.hpp"

class MLPredictor
{
public:
    explicit MLPredictor(std::shared_ptr<unsigned int> sys_tick,
                         std::shared_ptr<std::vector<std::unique_ptr<BasePhysicalChannel>>> channels,
                         double predict_error = 0.0);

    std::vector<double> predict_channel_conditions(unsigned int frequency, const std::vector<unsigned int>& powers);

    void seed_rng(uint64_t seed);

    std::shared_ptr<unsigned int> system_tick;

    std::shared_ptr<std::vector<std::unique_ptr<BasePhysicalChannel>>> channels;

    /* Half-width of the additive uniform noise applied to each predicted
       decode probability. 0 reproduces the noiseless oracle. */
    double predict_error;

private:
    std::default_random_engine generator;
};

