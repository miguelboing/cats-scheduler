#include <iostream>
#include <algorithm>
#include <numeric>

#include "cats.hpp"


CATS_scheduler::CATS_scheduler(unsigned int frequency, unsigned int rx_period, BufferPacket* buffer, std::shared_ptr<unsigned int> sys_tick): BaseScheduler(buffer, sys_tick), frequency(frequency), rx_period(rx_period)
{
    transmission_prob[0] = transmission_prob[1] = transmission_prob[2] = 0.0;
    retransmissions_per_frame = 1; /* TODO: estimate from channel prediction */
}

scheduled_frame_t CATS_scheduler::do_schedule_frame(void)
{
    scheduled_frame_t scheduled_frame;
    scheduled_frame.frequency = this->frequency;

    /* Check for packets to drop */
    std::vector<std::pair<unsigned int, unsigned int>> to_drop;
    for (auto& pkt : *this->buffer_packet)
    {
        unsigned int remaining_frames = pkt.frames - pkt.frame_count;
        unsigned int estimated_ticks  = remaining_frames * retransmissions_per_frame;
        if (pkt.deadline < *(this->system_tick) + estimated_ticks)
        {
            to_drop.emplace_back(pkt.id, pkt.id_count);
        }
    }
    for (auto& [id, id_count] : to_drop)
    {
        this->buffer->drop_packet(id, id_count);
    }

    if (*(this->system_tick) % this->rx_period == 0) /* Check if it is time to listen to the channel */
    {
        scheduled_frame.radio_mode = RX_MODE;
        scheduled_frame.packet = nullptr;
    }
    else /* If it is not try to schedule a packet */
    {
        /* Find the packet with the earliest deadline */
        auto lowest_it = std::min_element(this->buffer_packet->begin(),
                                          this->buffer_packet->end(),
                                          [](const packet_t& a, const packet_t& b) {
                                              return a.deadline < b.deadline;
                                          });
        if (lowest_it != this->buffer_packet->end())
        {
            scheduled_frame.packet = &(*lowest_it);
            scheduled_frame.radio_mode = TX_MODE;
        }
        else
        {
            scheduled_frame.packet = nullptr; /* Means idle/no tranmission */
            scheduled_frame.radio_mode = IDLE;
        }
    }

    return scheduled_frame;
}

void CATS_scheduler::receive_prediction(const std::vector<double>& pred_probs)
{
    std::copy(pred_probs.begin(), pred_probs.end(), this->transmission_prob);
}

std::string CATS_scheduler::get_name() const
{
    return "CATS Scheduler";
}

