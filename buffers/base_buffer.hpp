#pragma once

#include <vector>
#include <memory>
#include <iostream>
#include <algorithm>

#include "system_model/system_model.hpp"

/* This class keeps controls of the arriving packets, transmited packets and missed deadlines */
class BaseBuffer
{
public:
    std::shared_ptr<unsigned int> system_tick;
    std::shared_ptr<std::vector<packet_t>> buffer_packet;

    virtual ~BaseBuffer() = default;

    virtual void generate_packets() = 0;

    void update_buffer(packet_t& packet);

    void append_packet(packet_t& packet);

    virtual std::string get_name() const = 0;
};

inline void BaseBuffer::update_buffer(packet_t& scheduled_packet)
{
    /* Find packet on the buffer */
    auto packet_it = std::find_if(this->buffer_packet->begin(), this->buffer_packet->end(),
        [&scheduled_packet](const packet_t& p) { return &p == &scheduled_packet; });

    if (scheduled_packet.comp_cost > 0U) /* Check if this packet is valid */
    {
        /* Check if the packet is still on the deadline */
        if (scheduled_packet.deadline > *this->system_tick)
        {
            scheduled_packet.comp_cost = scheduled_packet.comp_cost - 1U;
        }
        else
        {
            std::cout << "Missed deadline from packet: ID ";
            std::cout << scheduled_packet.id << std::endl;
        }
    }
    else
    {
        /* This packet should have been erased! */
        std::cout << "This packet should have been removed previously!" << std::endl;
    }
    if (packet_it != buffer_packet->end() && scheduled_packet.comp_cost <= 0U)
    {
            this->buffer_packet->erase(packet_it);
    }

    /* Check for other missed deadlines */
    for (auto& packet: *this->buffer_packet)
    {
        /* Check if the packet missed its deadline */
        if (packet.deadline < *this->system_tick)
        {
            std::cout << "Missed packet ID: " << packet.id << std::endl;
            std::cout << "Missed packet deadline: " << packet.deadline << std::endl;
            std::cout << "Current time_frame: " << *this->system_tick << std::endl;
        }
    }

    /* Update the system tick */
    //(*this->system_tick)++;
}

inline void BaseBuffer::append_packet(packet_t& packet)
{
    this->buffer_packet->push_back(packet);
}

