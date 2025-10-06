#pragma once

#include <vector>
#include <memory>

typedef struct
{
//    unsigned int period;        /* T: Periodicy related with the task */
    unsigned int packet_id;     /* ID: Tasks unique identifier */
    unsigned int deadline;      /* D: This is the relative deadline */
    unsigned int comp_cost;     /* C: A unit consumes one frame */
    unsigned int success_rate;  /* S: Sureness that this packet was received */
} packet_t;

typedef struct
{
    unsigned int number_of_frames; /* This is the total number of frames available to transmit */
//    float channel_condition[number_of_frames][3]; /* Channel conditions for each frame at different power levels */

} system_model_t;

typedef struct {
    unsigned int task_id; /* task ID per frame */
    unsigned int tx_power;
} frame_allocation_t;

typedef struct {
    std::vector<frame_allocation_t> frame_allocation;
    bool is_feasible;                                  /* whether all deadlines met */
    std::vector<packet_t> missed_deadlines;            /* tasks that missed deadlines */
} schedule_result_t;


