#pragma once

#include <random>
#include <vector>

class Receiver
{
public:
    bool recv_packet(received_packet_t recv_packet);
private:
    std::default_random_engine generator;
};
