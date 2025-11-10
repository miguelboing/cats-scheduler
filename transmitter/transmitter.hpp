#pragma once

#include "system_model/system_model.hpp"

class Transmitter
{
public:
    std::shared_ptr<std::vector<packet_t>> buffer_packet;

    explicit Transmitter(std::shared_ptr<std::vector<packet_t>> buffer_packet):
       buffer_packet(buffer_packet) {};

    transmitted_packet_t transmit_frame(scheduled_packet_t scheduled_packet);
};
