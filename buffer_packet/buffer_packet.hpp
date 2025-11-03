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
    std::shared_ptr<std::vector<packet_t>> buffer_packet;

    std::shared_ptr<unsigned int> system_tick;

    BufferPacket(std::shared_ptr<unsigned int> system_tick):
        buffer_packet(std::make_shared<std::vector<packet_t>>(1, packet_t{0, 0, 0, 0, 0})),
        system_tick(system_tick) {};

    virtual ~BufferPacket() = default;

    void update_buffer(packet_t& packet);

    std::string get_name();
};

