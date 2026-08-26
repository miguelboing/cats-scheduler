#pragma once

#include <memory>
#include <unordered_map>

#include "schedulers/base_scheduler.hpp"

/* CHEDF -- CHARM's channel-aware retransmission/power policy over an EDF
   queue.

   Standalone by design: it stands on its own as a scheduler rather than as a
   parameterisation of CHARM. The consequence is that the retransmission rule
   below is a deliberate copy of CHARM's, not a shared one -- the CHARM/CHEDF
   comparison is only meaningful while the two differ solely in dequeue order,
   so any change to the accumulated-probability rule, the rx_period listening
   schedule or get_prediction_powers() must be applied to both files. */
class CHEDF_scheduler : public BaseScheduler
{
public:
    CHEDF_scheduler(unsigned int tx_power, unsigned int frequency, unsigned int rx_period, BufferPacket* buffer, std::shared_ptr<unsigned int> sys_tick);

    unsigned int tx_power;
    unsigned int frequency;
    unsigned int rx_period;
    double transmission_prob;
    std::unordered_map<uint64_t, double> accumulated_prob;

    scheduled_frame_t do_schedule_frame(void) override;

    void receive_prediction(const std::vector<double>& pred_probs) override;
    /* CHEDF transmits at a single fixed power, so it only ever needs the
       decode probability at that power. */
    std::vector<unsigned int> get_prediction_powers() const override { return {this->tx_power}; }

    std::string get_name() const override;
};
