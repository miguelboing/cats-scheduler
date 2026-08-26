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
    explicit BufferPacket(std::shared_ptr<unsigned int> sys_tick);

    std::shared_ptr<std::vector<packet_t>> buffer_packet;

    std::shared_ptr<unsigned int> system_tick;

    virtual ~BufferPacket() = default;

    std::vector<packet_t> check_deadlines(void);

    std::vector<packet_t> drop_packet(unsigned int id, unsigned int id_count);

    std::vector<packet_t> dropped_packets;

    static std::string get_name();
};

