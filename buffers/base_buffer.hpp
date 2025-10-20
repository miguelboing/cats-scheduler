#pragma once

#include <vector>
#include <memory>

#include "system_model/system_model.hpp"


/* This class keeps controls of the arriving packets, transmited packets and missed deadlines */
class BaseBuffer
{
public:
    std::shared_ptr<unsigned int> tick;
    std::shared_ptr<std::vector<packet_t>> buffer_packet;

    virtual ~BaseBuffer() = default;

    void update_buffer(packet_t& packet);

    virtual std::string get_name() const = 0;
}

inline void BaseBuffer::update_buffer(packet_t& scheduled_packet)
{
    /* Find packet on the buffer */
    auto packet_it = std::find_if(this->buffer_packet->begin(), this->buffer_packet->end(),
        [&packet](const packet_t& p) { return &p == &packet; });

    if (scheduled_packet.comp_cost > 0U) /* Check if this packet is valid */
    {
        /* Check if the packet is still on the deadline */
        if (scheduled_packet.deadline > this->system_tick)
        {
            scheduled_packet.comp_cost = scheduled_packet.comp_cost - 1U;
        }
        else
        {
            std::cout << "Missed deadline from packet";
	    std::cout << scheduled_packet.id << std::endl;
        }
    }
    else
    {
        /* This packet should have been erased! */
        std::cout << "This packet should have been removed previously!" << std::endl;
    }
    if (packet_it != buffer_packet->end() || scheduled_packet.comp_cost <= 0U)
        {
            buffer_packet->erase(packet_it);
        }

    /* Update the system tick */
    this->system_tick++;

    /* Check for other missed deadlines */
    for (auto& packet: this->buffer_packet)
    {
        /* Check if the packet missed its deadline */
        if (packet.deadline < this->system_tick)
        {
            std::cout << "Missed packet ID: " << packet.packet_id << std::endl;
            std::cout << "Missed packet deadline: " << packet.deadline << std::endl;
            std::cout << "Current time_frame: " << this->time_frame_counter << std::endl;
        }
    }

}
