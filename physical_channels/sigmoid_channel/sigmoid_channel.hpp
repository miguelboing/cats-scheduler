#pragma once

#include <kovian/kovian.hpp>

#include "physical_channels/base_physical_channel.hpp"

class MLPredictor;

class SigmoidChannel: public BasePhysicalChannel
{
friend class MLPredictor;
public:
    explicit SigmoidChannel(unsigned int frequency, const std::string& channel_name);

    double gen_probability(unsigned int transmission_power) override;
    received_frame_t gen_frame_with_probability(transmitted_frame_t transmitted_frame) override;
    void advance_fsmc_state(void);
    int get_fsmc_state(void);

    std::vector<markov_state_t> fsmc;
    kovian::MarkovChain<4> mc;

private:
    std::default_random_engine generator;
    double pathloss_db;     /* Pathloss of the channel in dB*/
};

