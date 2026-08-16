#include <memory>

#include "fixed_rate.hpp"

FixedRate_PacketGen::FixedRate_PacketGen(std::shared_ptr<unsigned int> system_tick, const std::vector<fixed_rate_packet_t>& packets, std::shared_ptr<std::vector<packet_t>> buffer_packet, std::shared_ptr<json> packet_gen_log):
    BasePacketGenerator(system_tick, buffer_packet, packet_gen_log), packets(packets) {};

void FixedRate_PacketGen::generate_packets(void)
{
    for (auto& packet : this->packets)
    {
        /* Check if any new packet has arrived */
        if (packet.phase + (packet.fixed_rate * packet.count) <= *this->system_tick)
        {
            /* Set a deadline */
            packet.original_packet.deadline = packet.phase + (packet.relative_deadline * ++packet.count);

            /* Set the period */
            packet.original_packet.is_periodic = true;
            packet.original_packet.period = packet.relative_deadline;

            this->add_packet_to_buffer(packet.original_packet);
         }
    }
}

std::string FixedRate_PacketGen::get_name(void) const
{
    return "FixedRate";
}

std::vector<periodic_task_t> FixedRate_PacketGen::get_periodic_tasks() const
{
    std::vector<periodic_task_t> out;
    out.reserve(this->packets.size());
    for (const auto& p : this->packets)
    {
        out.push_back({
            p.original_packet.id,
            p.fixed_rate,
            p.original_packet.frames,
            p.original_packet.reliability_req
        });
    }
    return out;
}

