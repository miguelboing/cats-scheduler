#include <cmath>
#include <iostream>

#include "sigmoid_channel.hpp"


SigmoidChannel::SigmoidChannel(unsigned int frequency, double ref_power_w, double ref_psr, double pathloss, double noise, double s):
    BasePhysicalChannel(frequency), slope(s), noise_floor_dbm(noise), pathloss_db(pathloss)
{
    /* Calculate SNR for reference power */
    double ref_tx_dbm = 10 * log10(ref_power_w * 1000);
    double ref_rx_dbm = ref_tx_dbm - pathloss_db;
    double ref_snr_db = ref_rx_dbm - noise_floor_dbm;

    /* Calculate SNR_50 based on reference PSR */
    this->snr_50_db = ref_snr_db + (1.0/slope) * log((1.0 - ref_psr)/ref_psr);
}

received_frame_t SigmoidChannel::gen_frame_with_probability(transmitted_frame_t transmitted_frame)
{
    received_frame_t recv_frame;
    recv_frame.packet = transmitted_frame.packet;
    recv_frame.transmission_power = transmitted_frame.transmission_power;
    recv_frame.frequency = transmitted_frame.frequency;

    /* Convert power W to dbmW */
    double tx_power_dbm = 10 * log10(recv_frame.transmission_power * 1000);

    /* Power after pathloss */
    double rx_power_dbm = tx_power_dbm - this->pathloss_db;

    /* SNR considering the noisefloor */
    double snr_db = rx_power_dbm - this->noise_floor_dbm;

    /* Calculating the probability on the sigmoid slope */
    recv_frame.success_prob = 1.0 / (1.0 + exp(-slope * (snr_db - snr_50_db)));

    return recv_frame;
}

