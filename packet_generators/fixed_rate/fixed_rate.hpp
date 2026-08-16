#pragma once

#include <memory>

#include "packet_generators/base_packet_generator.hpp"
#include "system_model/system_model.hpp"

struct fixed_rate_packet_t
{
    packet_t original_packet;

    /* Fixed rate specific parameters */
    unsigned int fixed_rate; /* Rate in system_tickets in which the packet will be spawned */
    unsigned int phase;      /* Offset from init time in which the packet will start to appear */
    unsigned int count;      /* Keeps track of packet spawn count */
    unsigned int relative_deadline;

    fixed_rate_packet_t(unsigned int relative_deadline, unsigned int frames, double reliability_req, unsigned int id, unsigned int fixed_rate, unsigned int phase) :
        fixed_rate(fixed_rate), phase(phase), relative_deadline(relative_deadline)
    {
        count = 0U;
        original_packet.deadline = 0U; /* Deadline is defined after the packet is spawned */
        original_packet.frames = frames;
        original_packet.reliability_req = reliability_req;
        original_packet.id = id;
   }

    /* Easy access to original fields */
    unsigned int& id()               { return original_packet.id; }
    unsigned int& deadline()         { return original_packet.deadline; }
    unsigned int& frames()           { return original_packet.frames; }
    double&       reliability_req() { return original_packet.reliability_req; }
};

class FixedRate_PacketGen: public BasePacketGenerator
{
public:
    FixedRate_PacketGen(std::shared_ptr<unsigned int> system_tick, const std::vector<fixed_rate_packet_t>& packets, std::shared_ptr<std::vector<packet_t>> buffer_packet, std::shared_ptr<json> packet_gen_log);
    void generate_packets(void) override;
    std::string get_name(void) const override;
    std::vector<periodic_task_t> get_periodic_tasks() const override;

private:
    std::vector<fixed_rate_packet_t> packets;
};

