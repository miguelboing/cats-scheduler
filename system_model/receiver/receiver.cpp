#include <random>
#include <vector>
#include <algorithm>
#include <iostream>

#include "system_model/system_model.hpp"
#include "receiver.hpp"

receiver_result_t Receiver::recv_packets(system_model_t system_model, schedule_result_t schedule_result)
{
    std::bernoulli_distribution distribution(0.0);
    receiver_result_t receiver_result;
    frame_allocation_t cur_frame;
    double frame_prob;
    bool prob_result; /* The result of the bernoulli prob function */

    /* Use the channel_condition to emulate packet loss */
    for (unsigned int i = 0U; i < system_model.number_of_frames; ++i)
    {
        /* Getting scheduled frame */
        cur_frame = schedule_result.frame_allocation[i];

        /* Selecting the channel */
        auto ch_it = std::find(system_model.frequencies.begin(), system_model.frequencies.end(), cur_frame.frequency);
        if (ch_it != system_model.frequencies.end())
        {
            size_t ch_idx = std::distance(system_model.frequencies.begin(), ch_it);
            auto& channel = (*system_model.channels)[ch_idx];

            /* Selecting the probability function based on power */
            auto pow_it = std::find(system_model.tx_power_levels.begin(), system_model.tx_power_levels.end(), cur_frame.tx_power);
            if (pow_it != system_model.tx_power_levels.end())
            {
                size_t pow_idx = std::distance(system_model.tx_power_levels.begin(), pow_it);
                frame_prob = (*channel.channel_condition)[pow_idx][i];
            }
        }

        distribution = std::bernoulli_distribution(frame_prob);
        prob_result = (distribution(this->generator));


        receiver_result.lost_frames.push_back(prob_result);

        if (prob_result)
        {
            receiver_result.received_frames.push_back('0' + (char)(schedule_result.frame_allocation[i].packet_id));
        }
        else
        {
            receiver_result.received_frames.push_back('X');
        }
    }

    return receiver_result;
}

