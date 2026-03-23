#include <iostream>
#include <algorithm>
#include <numeric>

#include "channel_aware_spf_scheduler.hpp"

CHASPF_scheduler::CHASPF_scheduler(unsigned int tx_power, unsigned int frequency, unsigned int rx_period, std::shared_ptr<std::vector<packet_t>> buffer_packet, std::shared_ptr<unsigned int> sys_tick): BaseScheduler(buffer_packet, sys_tick), tx_power(tx_power), frequency(frequency), rx_period(rx_period), transmission_prob(0) {};

scheduled_frame_t CHASPF_scheduler::do_schedule_frame(void)
{
    scheduled_frame_t scheduled_frame;
    scheduled_frame.transmission_power = this->tx_power;
    scheduled_frame.frequency = this->frequency;

    if (*(this->system_tick) % this->rx_period == 0) /* Check if it is time to listen to the channel */
    {
        scheduled_frame.radio_mode = RX_MODE;
        scheduled_frame.packet = nullptr;
    }
    else /* If it is not try to schedule a packet */
    {
        /* Find the packet with the smaller period */
        auto lowest_it = std::min_element(this->buffer_packet->begin(),
                                          this->buffer_packet->end(),
                                          [](const packet_t& a, const packet_t& b) {
                                              if (!a.is_periodic) return false;
                                              if (!b.is_periodic) return true;
                                              return a.period < b.period;
                                          });

        if (lowest_it != this->buffer_packet->end() && lowest_it->is_periodic)
        {
            scheduled_frame.packet = &(*lowest_it);
            scheduled_frame.radio_mode = TX_MODE;

            unsigned int key = lowest_it->id_count;

            /* Check if this frame is being transmitted for the first time */
            if (accumulated_prob.find(key) == accumulated_prob.end())
            {
                accumulated_prob[key] = this->transmission_prob;
            }
            else
            {
                /* Calculate the accumulated prob after this transmission */
                accumulated_prob[key] =
                    accumulated_prob[key] + this->transmission_prob - accumulated_prob[key] * this->transmission_prob;

            }

            /* Check if the prob is high enough to remove this frame from the buffer */
            if (accumulated_prob[key] >= lowest_it->success_rate_req)
            {
                scheduled_frame.remove_from_buffer = true;
                accumulated_prob.erase(key); /* requirement met, done with this packet */
            }
            else
            {
                scheduled_frame.remove_from_buffer = false;
            }
        }
        else
        {
            scheduled_frame.packet = nullptr; /* Means idle/no tranmission */
            scheduled_frame.radio_mode = IDLE;
        }
    }

    return scheduled_frame;
}

void CHASPF_scheduler::receive_prediction(double pred_dec_prob)
{
    this->transmission_prob = pred_dec_prob;
}

std::string CHASPF_scheduler::get_name() const {
    return "Channel Aware SPF";
}

