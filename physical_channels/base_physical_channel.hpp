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

    virtual received_packet_t gen_frame_with_probability(transmitted_packet_t transmitted_packet) = 0;

    unsigned int frequency;
};

