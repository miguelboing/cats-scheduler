#pragma once

#include <vector>
#include <memory>
#include <string>

typedef struct
{
//    unsigned int period;          /* T : Periodicy related with the task */
    unsigned int packet_id;         /* ID: Tasks unique identifier */
    unsigned int deadline;          /* D : This is the relative deadline */
    unsigned int comp_cost;         /* C : A unit consumes one frame */
    unsigned int success_rate_req;  /* S : Success rate requirement for the packet */
} packet_t;

struct channel_t
{
    unsigned int frequency;
    std::vector<unsigned int> tx_power_levels;
    std::shared_ptr<std::vector<std::vector<double>>> channel_condition; /* This is a 2D array frame x power */
    size_t num_power_levels;

    channel_t(unsigned int frequency, unsigned int num_frames, std::vector<unsigned int> tx_power_levels):
        frequency(frequency), tx_power_levels(tx_power_levels),
        num_power_levels(tx_power_levels.size())
    {
        channel_condition = std::make_shared<std::vector<std::vector<double>>>(
        num_power_levels, std::vector<double>(num_frames));
    }
};

struct system_model_t
{
    unsigned int number_of_frames;                     /* This is the total number of frames available to transmit */

    std::vector<unsigned int> tx_power_levels;         /* Possible TX Values for this system model */
    std::shared_ptr<std::vector<channel_t>> channels;  /* Vector with the different channels available for transmission */

    system_model_t(unsigned int num_frames, std::vector<unsigned int> frequencies, std::vector<unsigned int> tx_power_levels)
        : number_of_frames(num_frames), tx_power_levels(tx_power_levels),
          channels(std::make_shared<std::vector<channel_t>>())  /* 0 channels initially */
    {
        for (auto const& frequency: frequencies)
        {
            channels->push_back(channel_t(frequency, num_frames, tx_power_levels));
        }
    }
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

