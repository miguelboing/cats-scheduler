#include <memory>

#include "fixed_rate.hpp"

FixedRate_PacketGen::FixedRate_PacketGen(std::shared_ptr<unsigned int> system_tick, std::vector<fixed_rate_packet_t> packets, std::shared_ptr<std::vector<packet_t>> buffer_packet):
    BasePacketGenerator(system_tick, buffer_packet), packets(packets) {};

void FixedRate_PacketGen::generate_packets(void)
{
    for (auto& packet : packets)
    {
        /* Check if any new packet has arrived */
        if (packet.phase + (packet.fixed_rate * packet.count) <= *this->system_tick)
        {
            /* Set a deadline */
            packet.original_packet.deadline = packet.phase + (packet.relative_deadline * ++packet.count);

            /* Spawn a packet to the buffer */
            this->buffer_packet->push_back(packet.original_packet);
        }
    }
}

std::string FixedRate_PacketGen::get_name(void) const
{
    return "FixedRate";
}

