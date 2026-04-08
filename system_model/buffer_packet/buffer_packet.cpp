#include "buffer_packet.hpp"

BufferPacket::BufferPacket(std::shared_ptr<unsigned int> sys_tick):
        buffer_packet(std::make_shared<std::vector<packet_t>>()),
        system_tick(sys_tick) {};

std::vector<packet_t> BufferPacket::check_deadlines(void)
{
    std::vector<packet_t> missed_packets;

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
            missed_packets.push_back(*it);
            it = this->buffer_packet->erase(it);
        }
        else
        {
            ++it;
        }
    }
    return missed_packets;
}

std::vector<packet_t> BufferPacket::drop_packet(unsigned int id, unsigned int id_count)
{
    std::vector<packet_t> dropped;
    auto it = this->buffer_packet->begin();
    while (it != this->buffer_packet->end())
    {
        if (it->id == id && it->id_count == id_count)
        {
            dropped.push_back(*it);
            it = this->buffer_packet->erase(it);
        }
        else
        {
            ++it;
        }
    }
    this->dropped_packets.insert(this->dropped_packets.end(), dropped.begin(), dropped.end());
    return dropped;
}

std::string BufferPacket::get_name()
{
    return "BufferPacket";
}

