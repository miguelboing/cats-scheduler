#pragma once

#include <memory>
#include <vector>

#include "schedulers/base_scheduler.hpp"
#include "packet_generators/base_packet_generator.hpp"
#include <unordered_map>

class CATS_scheduler : public BaseScheduler
{
public:
    CATS_scheduler(unsigned int frequency,
                   float belief_threshold,
                   double utilization_threshold,
                   const std::vector<periodic_task_t>& periodic_tasks,
                   BufferPacket* buffer,
                   std::shared_ptr<unsigned int> sys_tick);

    unsigned int frequency;

    float belief;
    float belief_threshold;
    float eigenvalue;

    double transmission_prob[3];
    std::unordered_map<uint64_t, double> accumulated_prob;

    /* A-priori knowledge of the periodic task set (populated at construction).
       Used to size demand/utilization estimates against a fixed horizon. */
    std::vector<periodic_task_t> periodic_tasks;

    /* Utilization horizon — hyperperiod (LCM of periodic_tasks periods).
       Demand within one hyperperiod is exact since every task completes an
       integer number of releases. Zero when no periodic tasks exist. */
    unsigned int horizon_H;

    /* Slack-aware power-cap policy. On every channel prediction we recompute
       U at each predictor tier and cap the TX power at the lowest tier whose
       U <= utilization_threshold. Default cap is 25 W (index 2) so behavior
       before the first prediction is unchanged. */
    double utilization_threshold;
    unsigned int max_power_idx;

    scheduled_frame_t do_schedule_frame(void) override;

    void receive_prediction(const std::vector<double>& pred_probs) override;

    std::vector<unsigned int> get_prediction_powers() const override { return {1, 10, 25}; }

    std::string get_name() const override;

    /* Minimum number of transmissions per frame to achieve sr_req given a
       per-attempt success probability p. Returns +inf if p<=0<sr_req. */
    static double retx_count_required(double p, double sr_req);

    /* Demand in slots within the horizon H, given a per-frame success
       probability p. Demand_i = ceil(H/T_i) * C_i * retx_count_required(p, RR_i).
       Returns +inf if any task is infeasible at p. */
    double compute_demand_slots(double p) const;

    /* Utilization U = compute_demand_slots(p) / H. Returns +inf if infeasible,
       0.0 when there are no periodic tasks. */
    double compute_utilization(double p) const;
};

