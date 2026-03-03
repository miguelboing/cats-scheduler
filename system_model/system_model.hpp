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
    unsigned int frame_count;       /* FC: The amount of frames transmitted from this packet_count */
    double       success_rate_req;  /* S : Success rate requirement for the packet */
}
packet_t;

typedef enum
{
    IDLE=0,
    TX_MODE,
    RX_MODE
} radio_mode_e;

typedef struct
{
    packet_t* packet;
    unsigned int transmission_power;
    unsigned int frequency;
    radio_mode_e radio_mode;
}
scheduled_frame_t;

typedef struct
{
    packet_t packet;
    unsigned int transmission_power;
    unsigned int frequency;
}
transmitted_frame_t;

typedef struct
{
    packet_t packet;
    unsigned int transmission_power;
    unsigned int frequency;
    double success_prob;
}
received_frame_t;

