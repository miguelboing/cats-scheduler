#pragma once

#include <vector>
#include <memory>
#include <iostream>
#include <algorithm>
#include <unordered_map>

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

protected:
    void add_packet_to_buffer(packet_t& packet);
    std::unordered_map<unsigned int, unsigned int> packet_count_map; /* Maps ID-> count */
};

inline void BasePacketGenerator::add_packet_to_buffer(packet_t& packet)
{
    /* Set packet count, auto-initializes to 0 if new ID */
    packet.id_count = this->packet_count_map[packet.id]++;
    packet.frame_count = 0U;

    /* Spawn a packet to the buffer */
    this->buffer_packet->push_back(packet);
};

