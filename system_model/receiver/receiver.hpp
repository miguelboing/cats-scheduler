#pragma once

#include <random>
#include <vector>

class Receiver
{
public:
    bool recv_frame(received_frame_t recv_frame);
private:
    std::default_random_engine generator;
};
