#pragma once

#include <vector>
#include <memory>
#include <string>

typedef struct
{
    unsigned int id;                /* ID: Tasks unique identifier */
    unsigned int id_count;          /* PC: Identifier between packets with the same ID */
    unsigned int deadline;          /* D : This is the relative deadline */
    unsigned int frames;            /* C : A unit consumes one frame */
    unsigned int frame_count;
    unsigned int success_rate_req;  /* S : Success rate requirement for the packet */
} packet_t;

struct channel_t
{
    unsigned int frequency;
    std::vector<unsigned int> tx_power_levels;
    std::shared_ptr<std::vector<std::vector<double>>> channel_condition; /* This is a 2D array frame x power */
    size_t num_power_levels;

    channel_t(unsigned int frequency, unsigned int num_frames, const std::vector<unsigned int>& tx_power_levels):
        frequency(frequency), tx_power_levels(tx_power_levels),
        num_power_levels(tx_power_levels.size())
    {
        channel_condition = std::make_shared<std::vector<std::vector<double>>>(
        num_power_levels, std::vector<double>(num_frames));
    }
};

typedef struct
{
    unsigned int number_of_frames;                     /* This is the total number of frames available to transmit */
    std::vector<unsigned int> frequencies;             /* List of frequencies available for transmission */
} system_model_t;

typedef struct
{
    packet_t* packet;
    unsigned int transmission_power;
    unsigned int frequency;
}
scheduled_packet_t;

typedef struct
{
    packet_t packet;
    unsigned int transmission_power;
    unsigned int frequency;
}
transmitted_packet_t;

typedef struct
{
    packet_t packet;
    unsigned int transmission_power;
    unsigned int frequency;
    double success_prob;
}
received_packet_t;

