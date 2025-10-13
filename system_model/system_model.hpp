#pragma once

#include <vector>
#include <memory>

typedef struct
{
//    unsigned int period;          /* T : Periodicy related with the task */
    unsigned int packet_id;         /* ID: Tasks unique identifier */
    unsigned int deadline;          /* D : This is the relative deadline */
    unsigned int comp_cost;         /* C : A unit consumes one frame */
    unsigned int success_rate_req;  /* S : Success rate requirement for the packet */
} packet_t;

struct system_model_t
{
    unsigned int number_of_frames;                          /* This is the total number of frames available to transmit */
    std::shared_ptr<std::vector<double>> channel_condition; /* Channel conditions for each frame at different power levels */

    system_model_t(unsigned int num_frames)
        : number_of_frames(num_frames),
          channel_condition(std::make_shared<std::vector<double>>(num_frames))
    {}

};

typedef struct {
    unsigned int packet_id;        /* Packet ID per frame */
    unsigned int packet_frame_id;  /* The frame count for each packet */
    unsigned int packet_count;     /* This is the counter of how many packets have been sent using this packet_id disregarding packet_frame_id */
    unsigned int tx_power;
} frame_allocation_t;

typedef struct {
    std::vector<frame_allocation_t> frame_allocation;
    bool is_feasible;                                  /* whether all deadlines met */
    std::vector<packet_t> missed_deadlines;            /* tasks that missed deadlines */
} schedule_result_t;

typedef struct {
    std::vector<char> received_frames;
    std::vector<bool> lost_frames;
} receiver_result_t;

