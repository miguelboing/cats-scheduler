#pragma once

#include <vector>
#include <memory>
#include <iostream>
#include <algorithm>

#include "system_model/system_model.hpp"

/* This class keeps controls of the arriving packets, transmited packets and missed deadlines */
class BufferPacket
{
public:
    std::shared_ptr<unsigned int> system_tick;

    std::shared_ptr<std::vector<packet_t>> buffer_packet;

    BufferPacket(std::shared_ptr<unsigned int> system_tick):
        system_tick(system_tick)
    {
        /* Initialize buffer_packet with IDLE PACKET */
        buffer_packet = std::make_shared<std::vector<packet_t>>(1, packet_t{0, 0, 0, 0});
    }

    virtual ~BufferPacket() = default;

    void update_buffer(packet_t& packet);

    std::string get_name();
};

