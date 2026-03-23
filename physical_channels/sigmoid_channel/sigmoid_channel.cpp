#include <cmath>
#include <iostream>
#include <fstream>
#include <random>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <kovian/kovian.hpp>
using namespace kovian::aliases;

#include "sigmoid_channel.hpp"

SigmoidChannel::SigmoidChannel(unsigned int frequency, const std::string& channel_name):
    BasePhysicalChannel(frequency), mc("physical_channels/" + channel_name + "/transition_matrix.kov")
{
    this->pathloss_db = 0;

    /* Loading States */
    std::ifstream f("physical_channels/" + channel_name + "/fsmc_states.json");

    json data = json::parse(f);
    for (const auto& s : data)
    {
        fsmc.push_back
        ({
            s["snr_50_db"],
            s["slope"],
            s["max_saturation"],
            s["noise_floor_dbm"]
        });
    }

    /* Defining initial state for fsmc */
    std::uniform_int_distribution<int> distribution(0, 4);
    this->mc.setState(distribution(generator));
}

double SigmoidChannel::gen_probability(unsigned int transmission_power)
{
    int fsmc_state;
    double tx_power_dbm, rx_power_dbm, snr_db;

    /* Getting the current state */
    fsmc_state = this->mc.current();

    /* Convert power W to dbmW */
    tx_power_dbm = 10 * log10(transmission_power * 1000);

    /* Power after pathloss */
    rx_power_dbm = tx_power_dbm - this->pathloss_db;

    /* SNR considering the noisefloor */
    snr_db = rx_power_dbm - this->fsmc[fsmc_state].noise_floor_dbm;

    /* Calculating the probability on the sigmoid slope */
    return 1.0 / (1.0 + exp(-(this->fsmc[fsmc_state].slope) * (snr_db - (this->fsmc[fsmc_state].snr_50_db))));
}

received_frame_t SigmoidChannel::gen_frame_with_probability(transmitted_frame_t transmitted_frame)
{
    received_frame_t recv_frame;
    recv_frame.packet = transmitted_frame.packet;
    recv_frame.transmission_power = transmitted_frame.transmission_power;
    recv_frame.frequency = transmitted_frame.frequency;

    recv_frame.success_prob = this->gen_probability(recv_frame.transmission_power);

    return recv_frame;
}

void SigmoidChannel::advance_fsmc_state(void)
{
    (void)this->mc.advance();
}

int SigmoidChannel::get_fsmc_state(void)
{
    return this->mc.current();
}

