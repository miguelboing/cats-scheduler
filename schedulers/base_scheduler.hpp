#pragma once

#include <vector>
#include <memory>

#include "system_model/system_model.hpp"

class BaseScheduler {
public:
    virtual ~BaseScheduler() = default;

    void add_queue(std::shared_ptr<std::vector<packet_t>> queue_packet);

    void reset_scheduler();

    /* Pure virtual functions that all schedulers must implement */
    virtual schedule_result_t schedule_packets(system_model_t system_model) = 0;

    virtual std::string get_name() const = 0;

    std::shared_ptr<std::vector<packet_t>> queue_packet; /* Vector that points to the queue to be scheduled */

    schedule_result_t schedule_result;

protected:
    unsigned int time_frame_counter = 0U;

    std::shared_ptr<std::vector<packet_t>> priv_queue_packet; /* Internal variable to handle iterations of queue_packet*/
};

inline void BaseScheduler::add_queue(std::shared_ptr<std::vector<packet_t>> queue_packet) {
    this->queue_packet = queue_packet; /* Keep an untouched reference to the original queue */
    this->priv_queue_packet = std::make_shared<std::vector<packet_t>>(*queue_packet); /* Make a copy of the queue to update deadlines */
}

inline void BaseScheduler::reset_scheduler() {
    this->priv_queue_packet = this->queue_packet;
}

