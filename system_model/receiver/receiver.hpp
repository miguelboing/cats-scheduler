#pragma once

#include <random>
#include <vector>

class Receiver
{
public:
    receiver_result_t recv_packets(system_model_t system_model, schedule_result_t schedule_result);
private:
    std::default_random_engine generator;
};
