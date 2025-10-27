#include "buffer_packet.hpp"

void BufferPacket::update_buffer(packet_t& scheduled_packet)
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
}

std::string BufferPacket::get_name()
{
    return "BufferPacket";
}
