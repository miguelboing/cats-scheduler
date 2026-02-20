#pragma once

#include "physical_channels/base_physical_channel.hpp"

class MLPredictor;

class SigmoidChannel: public BasePhysicalChannel
{

friend class MLPredictor;
public:
    explicit SigmoidChannel(unsigned int frequency, double ref_power_w=10, double ref_psr=0.8, double pathloss=100, double noise = -90.0, double s = 2.0);

    double gen_probability(unsigned int transmission_power) override;
    received_frame_t gen_frame_with_probability(transmitted_frame_t transmitted_frame) override;

private:
    double snr_50_db;       /* beta: SNR for 50% success rate */
    double slope;           /* alpha: Steepness of sigmoid curve */
    double max_saturation   /* gamma: The maximum possible transmission power when tx_power -> inf */

    double noise_floor_dbm; /* Noise power in dBm */
    double pathloss_db;     /* Pathloss of the channel in dB*/
};
