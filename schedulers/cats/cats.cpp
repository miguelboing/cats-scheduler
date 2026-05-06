#include <iostream>
#include <algorithm>
#include <numeric>

#include "cats.hpp"


CATS_scheduler::CATS_scheduler(unsigned int frequency, float belief_threshold, double margin, BufferPacket* buffer, std::shared_ptr<unsigned int> sys_tick): BaseScheduler(buffer, sys_tick), frequency(frequency), belief_threshold(belief_threshold), margin(margin)
{
    this->transmission_prob[0] = this->transmission_prob[1] = this->transmission_prob[2] = 0.0;
    this->belief = 0.0;
    this->retransmissions_per_frame = 1U; /* TODO: estimate from channel prediction */
    this->eigenvalue = 0.99015;
}

scheduled_frame_t CATS_scheduler::do_schedule_frame(void)
{
    scheduled_frame_t scheduled_frame;
    scheduled_frame.frequency = this->frequency;

    /* Drop unfeasible packets and detect urgent ones in the same pass.
       Under belief-based scheduling any slot can be TX, so a packet is
       droppable iff ticks_available < needed_slots, and urgent iff
       ticks_available == needed_slots (every remaining slot must be TX —
       listening once would force a drop next iteration). */
    std::vector<std::pair<unsigned int, unsigned int>> to_drop;
    bool no_urgent_packet = true;
    for (auto& pkt : *this->buffer_packet)
    {
        if (pkt.deadline <= *(this->system_tick)) continue; /* already expired, check_deadlines handles it */
        unsigned int ticks_available  = pkt.deadline - *(this->system_tick);
        unsigned int remaining_frames = pkt.frames - pkt.frame_count;
        unsigned int needed_slots     = remaining_frames * this->retransmissions_per_frame;
        if (ticks_available < needed_slots)
        {
            to_drop.emplace_back(pkt.id, pkt.id_count);
        }
        else if (ticks_available == needed_slots)
        {
            no_urgent_packet = false;
        }
    }
    for (auto& [id, id_count] : to_drop)
    {
        accumulated_prob.erase(id_count);
        this->buffer->drop_packet(id, id_count);
    }

    /* Listen only if we don't trust the channel AND no packet is on the edge of its deadline */
    if (this->belief < this->belief_threshold && no_urgent_packet)
    {
        scheduled_frame.radio_mode = RX_MODE;
        scheduled_frame.packet = nullptr;
        this->belief = 1.0;
    }
    else /* If it isn't, try to schedule a packet */
    {
        /* Belief drops */
        this->belief *= this->eigenvalue;

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

            /* Margin protects against single-shot Bernoulli failures: with no ack we
               can't recover a missed draw, so pick the smallest power whose accumulated
               probability clears req + margin (clamped at 1.0). */
            const double effective_req = std::min(1.0, req + this->margin);
            int chosen_idx = -1;
            for (int i = 0; i < 3; i++)
            {
                double acc_after = acc + transmission_prob[i] - acc * transmission_prob[i];
                if (acc_after >= effective_req)
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
            scheduled_frame.radio_mode = RX_MODE;
            this->belief = 1.0;
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

