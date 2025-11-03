#pragma once

#include <vector>
#include <memory>

#include "system_model/system_model.hpp"

class BaseScheduler
{
public:
    BaseScheduler(std::shared_ptr<std::vector<packet_t>> buffer);

    virtual ~BaseScheduler() = default;

    void reset_scheduler();

    void change_buffer_packet(std::shared_ptr<std::vector<packet_t>> buffer);

    /* Pure virtual functions that all schedulers must implement */
    virtual packet_t& schedule_packets(void) = 0;

    virtual std::string get_name() const = 0;

    std::shared_ptr<std::vector<packet_t>> buffer_packet; /* Vector that points to the queue to be scheduled */

    schedule_result_t schedule_result;

protected:
    unsigned int time_frame_counter = 0U;

    std::shared_ptr<std::vector<packet_t>> priv_queue_packet; /* Internal variable to handle iterations of queue_packet*/
};

inline BaseScheduler::BaseScheduler(std::shared_ptr<std::vector<packet_t>> buffer)
{
    this->change_buffer_packet(buffer);
}

inline void BaseScheduler::change_buffer_packet(std::shared_ptr<std::vector<packet_t>> buffer) {
    this->buffer_packet = buffer; /* Keep an untouched reference to the original queue */
}

