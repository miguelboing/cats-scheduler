#pragma once

#include <random>
#include <vector>

#include "system_model/system_model.hpp"

#include "physical_channels/sigmoid_channel/sigmoid_channel.hpp"

class MLPredictor
{
public:
    explicit MLPredictor(std::shared_ptr<unsigned int> sys_tick, std::shared_ptr<std::vector<SigmoidChannel>> sigmoid_channels);

    double predict_channel_conditions(unsigned int frequency, unsigned int transmission_power);

    std::shared_ptr<unsigned int> system_tick;

    std::shared_ptr<std::vector<SigmoidChannel>> channels;
};

