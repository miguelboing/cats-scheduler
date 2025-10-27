#pragma once

#include <vector>
#include <memory>
#include <iostream>
#include <algorithm>

#include "system_model/system_model.hpp"

/* This class keeps controls of the arriving packets, transmited packets and missed deadlines */
class BasePacketGenerator
{
public:
    std::shared_ptr<unsigned int> system_tick;

    std::shared_ptr<std::vector<packet_t>> buffer_packet; /* This points to the buffer */

    BasePacketGenerator(std::shared_ptr<unsigned int> system_tick, std::shared_ptr<std::vector<packet_t>> buffer_packet):
        system_tick(system_tick), buffer_packet(buffer_packet) {};

    virtual ~BasePacketGenerator() = default;

    virtual void generate_packets(void) = 0;

    virtual std::string get_name() const = 0;
};
