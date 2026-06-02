#pragma once

#include <vector>
#include <memory>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <unordered_map>

#include <nlohmann/json.hpp>
using json = nlohmann::json;



#include "system_model/system_model.hpp"

/* Minimal periodic-task descriptor exposed by generators that produce
   periodic traffic. Used by schedulers that need a-priori knowledge of the
   task set (e.g., CATS computing demand/utilization over a horizon). */
struct periodic_task_t
{
    unsigned int id;
    unsigned int period;            /* T_i: inter-arrival in ticks */
    unsigned int frames;            /* C_i: frames per release */
    double       success_rate_req;  /* SR_i: required reception probability */
};

/* This class keeps controls of the arriving packets, transmited packets and missed deadlines */
class BasePacketGenerator
{
public:
    std::shared_ptr<unsigned int> system_tick;

    std::shared_ptr<std::vector<packet_t>> buffer_packet; /* This points to the buffer */

    std::shared_ptr<json> packet_gen_log;

    BasePacketGenerator(std::shared_ptr<unsigned int> system_tick, std::shared_ptr<std::vector<packet_t>> buffer_packet, std::shared_ptr<json> packet_gen_log):
        system_tick(system_tick), buffer_packet(buffer_packet), packet_gen_log(packet_gen_log) {};

    virtual ~BasePacketGenerator() = default;

    virtual void generate_packets(void) = 0;

    virtual std::string get_name() const = 0;

    /* Generators with periodic traffic expose their task set here; default
       is empty so non-periodic generators don't need to override. */
    virtual std::vector<periodic_task_t> get_periodic_tasks() const { return {}; }

    /* Toggle per-spawn logging into packet_gen_log. Disable in summary mode
       to keep memory flat across long-duration runs. */
    void set_log_enabled(bool enabled) { log_enabled = enabled; }

    static void save_to_file(std::shared_ptr<json> log, const std::string& filename);

protected:
    void add_packet_to_buffer(packet_t& packet);
    std::unordered_map<unsigned int, unsigned int> packet_count_map; /* Maps ID-> count */
    bool log_enabled = true;
};

inline void BasePacketGenerator::add_packet_to_buffer(packet_t& packet)
{
    /* Set packet count, auto-initializes to 0 if new ID */
    packet.id_count = this->packet_count_map[packet.id]++;
    packet.frame_count = 0U;

    /* Log the spawn event */
    if (this->log_enabled)
    {
        json spawn_entry = {
            {"system_tick", *(this->system_tick)},
            {"packet_id", packet.id},
            {"packet_id_count", packet.id_count},
            {"deadline", packet.deadline},
            {"frames", packet.frames},
            {"success_rate_req", packet.success_rate_req},
            {"is_periodic", packet.is_periodic},
            {"period", packet.period},
            {"generator_type", this->get_name()}  /* Track which generator spawned it */
        };
        this->packet_gen_log->push_back(spawn_entry);
    }

    /* Spawn a packet to the buffer */
    this->buffer_packet->push_back(packet);
};

inline void BasePacketGenerator::save_to_file(std::shared_ptr<json> log, const std::string& filename)
{
    std::ofstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return;
    }
    file << log->dump(4);
    file.close();
}

