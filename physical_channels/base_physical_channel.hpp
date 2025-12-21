#pragma once

#include <vector>
#include <memory>
#include <random>

#include "system_model/system_model.hpp"

class BasePhysicalChannel
{
public:
    explicit BasePhysicalChannel(unsigned int frequency):
        frequency(frequency) {};

    virtual received_frame_t gen_frame_with_probability(transmitted_frame_t transmitted_frame) = 0;

    unsigned int frequency;
};

