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

    void check_deadlines(void);

    static std::string get_name();
};

