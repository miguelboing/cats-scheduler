#include <random>
#include <vector>
#include <iostream>

#include "system_model/system_model.hpp"
#include "receiver.hpp"

receiver_result_t Receiver::recv_packets(system_model_t system_model, schedule_result_t schedule_result)
{
    std::bernoulli_distribution distribution(0.0);
    receiver_result_t receiver_result;
    bool prob_result; /* The result of the bernoulli prob function */

    /*TODO: Change the probabilities based on the transmission power */

    /* Use the channel_condition to emulate packet loss */
    for (unsigned int i=0; i< system_model.number_of_frames; ++i)
    {
        distribution = std::bernoulli_distribution((*system_model.channel_condition)[i]);
        prob_result = (distribution(this->generator));

        receiver_result.lost_frames.push_back(prob_result);

        if (prob_result)
        {
            receiver_result.received_frames.push_back('0' + (char)(schedule_result.frame_allocation[i].task_id));
        }
        else
        {
            receiver_result.received_frames.push_back('X');
        }
    }

    return receiver_result;
}

