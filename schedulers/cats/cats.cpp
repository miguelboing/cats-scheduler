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

    /* Check for packets to drop — use ceiling division for tx_slots to avoid premature drops */
    std::vector<std::pair<unsigned int, unsigned int>> to_drop;
    for (auto& pkt : *this->buffer_packet)
    {
        if (pkt.deadline <= *(this->system_tick)) continue; /* already expired, check_deadlines handles it */
        unsigned int ticks_available  = pkt.deadline - *(this->system_tick);
        /* Ceiling division: floor((T * (P-1) + P-1) / P) */
        unsigned int tx_slots         = (ticks_available * (this->rx_period - 1) + this->rx_period - 1) / this->rx_period;
        unsigned int remaining_frames = pkt.frames - pkt.frame_count;
        if (tx_slots < remaining_frames * retransmissions_per_frame)
        {
            to_drop.emplace_back(pkt.id, pkt.id_count);
        }
    }
    for (auto& [id, id_count] : to_drop)
    {
        accumulated_prob.erase(id_count);
        this->buffer->drop_packet(id, id_count);
    }

    /* Check if it is time to listen to the channel */
    if (*(this->system_tick) % this->rx_period == 0)
    {
        scheduled_frame.radio_mode = RX_MODE;
        scheduled_frame.packet = nullptr;
    }
    else /* If it isn't, try to schedule a packet */
    {
        /* Find the packet with the earliest deadline */
        auto lowest_it = std::min_element(this->buffer_packet->begin(),
                                          this->buffer_packet->end(),
                                          [](const packet_t& a, const packet_t& b) {
                                              return a.deadline < b.deadline;
                                          });

        if (lowest_it != this->buffer_packet->end())
        {
            const unsigned int power_levels[3] = {1, 10, 25};
            unsigned int key = lowest_it->id_count;
            double req       = lowest_it->success_rate_req;

            /* Get or initialise accumulated probability for this packet instance */
            double acc = (accumulated_prob.find(key) != accumulated_prob.end())
                         ? accumulated_prob[key] : 0.0;

            /* Pick smallest power whose accumulated probability would meet the requirement */
            int chosen_idx = -1;
            for (int i = 0; i < 3; i++)
            {
                double acc_after = acc + transmission_prob[i] - acc * transmission_prob[i];
                if (acc_after >= req)
                {
                    chosen_idx = i;
                    break;
                }
            }

            if (chosen_idx >= 0)
            {
                /* Accumulated probability meets requirement — remove from buffer */
                accumulated_prob[key] = acc + transmission_prob[chosen_idx]
                                        - acc * transmission_prob[chosen_idx];
                scheduled_frame.transmission_power = power_levels[chosen_idx];
                scheduled_frame.remove_from_buffer = true;
                accumulated_prob.erase(key);
            }
            else
            {
                /* No power level meets requirement yet — use max power, keep retransmitting */
                accumulated_prob[key] = acc + transmission_prob[2] - acc * transmission_prob[2];
                scheduled_frame.transmission_power = power_levels[2];
                scheduled_frame.remove_from_buffer = false;
            }

            scheduled_frame.packet     = &(*lowest_it);
            scheduled_frame.radio_mode = TX_MODE;
        }
        else
        {
            scheduled_frame.packet = nullptr;
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
