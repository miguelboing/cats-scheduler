#include <memory>

#include "fixed_rate.hpp"

FixedRate_buffer::FixedRate_buffer(std::vector<fixed_rate_packet_t> packets): packets(packets) {};

void FixedRate_buffer::generate_packets(void)
{
    for (auto& packet : packets)
    {
        /* Check if any new packet has arrived */
        if (packet.phase + (packet.fixed_rate * packet.count) >= *this->system_tick)
        {
            /* Set a deadline */
            packet.original_packet.deadline = packet.phase + (packet.fixed_rate * ++packet.count);

            /* Spawn a packet to the buffer */
            this->append_packet(packet.original_packet);
        }
    }
}

std::string FixedRate_buffer::get_name(void) const
{
    return "FixedRate";
}

