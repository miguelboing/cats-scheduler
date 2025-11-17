#pragma once

#include "physical_channels/base_physical_channel.hpp"

class SigmoidChannel: public BasePhysicalChannel
{
public:
    explicit SigmoidChannel(unsigned int frequency, double ref_power_w=10, double ref_psr=0.8, double pathloss=100, double noise = -90.0, double s = 2.0);

    received_packet_t gen_frame_with_probability(transmitted_packet_t transmitted_packet) override;

private:
    double snr_50_db;       /* SNR for 50% success rate */
    double slope;           /* Steepness of sigmoid curve */
    double noise_floor_dbm; /* Noise power in dBm */
    double pathloss_db;     /* Pathloss of the channel in dB*/
};
