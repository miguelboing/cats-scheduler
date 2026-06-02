#pragma once

#include <memory>
#include <random>
#include <vector>

#include "system_model/system_model.hpp"
#include "physical_channels/base_physical_channel.hpp"

class MLPredictor
{
public:
    explicit MLPredictor(std::shared_ptr<unsigned int> sys_tick,
                         std::shared_ptr<std::vector<std::unique_ptr<BasePhysicalChannel>>> channels);

    std::vector<double> predict_channel_conditions(unsigned int frequency, const std::vector<unsigned int>& powers);

    std::shared_ptr<unsigned int> system_tick;

    std::shared_ptr<std::vector<std::unique_ptr<BasePhysicalChannel>>> channels;
};

