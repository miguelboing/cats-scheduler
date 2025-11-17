#include <random>
#include <vector>
#include <algorithm>
#include <iostream>

#include "system_model/system_model.hpp"
#include "receiver.hpp"

bool Receiver::recv_frame(received_frame_t recv_frame)
{
    std::bernoulli_distribution distribution(0.0);

    distribution = std::bernoulli_distribution(recv_frame.success_prob);
    bool prob_result = distribution(this->generator);

    return prob_result;
}

