#include <random>
#include <vector>
#include <iostream>

#include "system_model/system_model.hpp"
#include "receiver.hpp"

void Receiver::recv_packets(system_model_t system_model, schedule_result_t schedule_result)
{
    std::vector<bool> lost_frames;
    std::bernoulli_distribution distribution(0.0);

    /*TODO: Change the probabilities based on the transmission power */

    /* Use the channel_condition to emulate packet loss */
    for (const auto &channel_prob: *system_model.channel_condition)
    {
        distribution = std::bernoulli_distribution(channel_prob);
        lost_frames.push_back(distribution(this->generator));
        std::cout << channel_prob << ", " << lost_frames.back() << " |" ;
    }

    std::cout << std::endl;
}

