#include <random>
#include <vector>
#include <algorithm>
#include <iostream>

#include "system_model/system_model.hpp"
#include "receiver.hpp"

bool Receiver::recv_packet(received_packet_t recv_packet)
{
    std::bernoulli_distribution distribution(0.0);

    distribution = std::bernoulli_distribution(recv_packet.success_prob);
    bool prob_result = distribution(this->generator);

    return prob_result;
}

