#include "buffer_packet.hpp"

BufferPacket::BufferPacket(std::shared_ptr<unsigned int> sys_tick):
        buffer_packet(std::make_shared<std::vector<packet_t>>()),
        system_tick(sys_tick) {};

void BufferPacket::check_deadlines(void)
{
    /* Check each packet state */
    auto it = this->buffer_packet->begin();
    while (it != this->buffer_packet->end())
    {
        if (it->frames <= it->frame_count)
        {
            it = this->buffer_packet->erase(it);
        }
        else if (it->deadline < *this->system_tick)
        {
            std::cout << "Missed packet ID: " << it->id << std::endl;
            std::cout << "Missed packet ID Counter: " << it->id_count << std::endl;
            std::cout << "Missed packet deadline: " << it->deadline << std::endl;
            std::cout << "Current time_frame: " << *this->system_tick << std::endl;
            ++it;
        }
        else
        {
            ++it;
        }
    }
}

std::string BufferPacket::get_name()
{
    return "BufferPacket";
}

