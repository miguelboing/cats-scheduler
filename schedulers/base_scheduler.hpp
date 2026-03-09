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
    virtual scheduled_frame_t schedule_frame(void) = 0;

    virtual std::string get_name() const = 0;

    /* Non Pure virtual function */
    virtual void receive_prediction(double pred_dec_prob); /* This function handles the channel predictions */

    std::shared_ptr<std::vector<packet_t>> buffer_packet; /* Vector that points to the queue to be scheduled */
};

inline BaseScheduler::BaseScheduler(std::shared_ptr<std::vector<packet_t>> buffer)
{
    this->change_buffer_packet(buffer);
}

inline void BaseScheduler::change_buffer_packet(std::shared_ptr<std::vector<packet_t>> buffer)
{
    this->buffer_packet = buffer; /* Keep an untouched reference to the original queue */
}

inline void BaseScheduler::receive_prediction(double pred_dec_prob)
{
    return NULL;
}

