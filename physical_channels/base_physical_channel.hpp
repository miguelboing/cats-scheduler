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

    virtual ~BasePhysicalChannel() = default;

    virtual received_frame_t gen_frame_with_probability(transmitted_frame_t transmitted_frame) = 0;
    virtual double gen_probability(unsigned int transmission_power) = 0;

    /* Per-tick FSMC step. Default no-op for channels without internal state. */
    virtual void advance_fsmc_state(void) {}

    unsigned int frequency;
};

