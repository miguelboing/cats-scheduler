#pragma once

#include <memory>

#include "schedulers/base_scheduler.hpp"

struct edf_packet_t {
    packet_t original_packet;  /* The original packet data */

    /* EDF-specific internal parameters */
    unsigned int original_deadline;    /* Store original deadline for reset */
    unsigned int remaining_comp_cost;  /* Remaining computation time */
    bool         is_available;         /* Whether packet is currently scheduled */
    unsigned int last_execution_frame; /* Last frame when this packet was executed */
    unsigned int period;               /* Period for periodic tasks */
    unsigned int packet_count;         /* Number of times this packet has been transmited */

    /* Constructor */
    edf_packet_t(const packet_t& packet) : original_packet(packet) {
        original_deadline = packet.deadline;
        remaining_comp_cost = packet.comp_cost;
        is_available = true;
        period = packet.deadline; /* Assume deadline = period initially */
        packet_count = 0;
    }

    /* Easy access to original fields */
    unsigned int& id()               { return original_packet.id; }
    unsigned int& deadline()         { return original_packet.deadline; }
    unsigned int& comp_cost()        { return original_packet.comp_cost; }
    unsigned int& success_rate_req() { return original_packet.success_rate_req; }
};

class EDF_scheduler : public BaseScheduler {
public:
    EDF_scheduler(unsigned int tx_power, unsigned int frequency);
    unsigned int tx_power;
    unsigned int frequency;
    schedule_result_t schedule_packets(system_model_t system_model) override;

    std::string get_name() const override;
private:
      std::vector<edf_packet_t> edf_packets;
//      static const edf_packet_t idle_task; /* This is run when no other task is available*/
};





