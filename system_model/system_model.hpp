#pragma once

#include <vector>
#include <memory>
#include <string>

typedef struct
{
    unsigned int id;                  /* ID: Tasks unique identifier */
    unsigned int id_count;            /* PC: Identifier between packets with the same ID */
    unsigned int deadline;            /* D : This is the relative deadline */
    unsigned int frames;              /* C : A unit consumes one frame */
    unsigned int frame_count;         /* FC: The amount of frames transmitted from this packet_count */
    double       success_rate_req;    /* S : Success rate requirement for the packet */
    bool         is_periodic = false; /* Flag to warn the scheduler if this packet is periodic or not */
    unsigned int period;              /* P: If is_periodic is true, this value represents the period of the packet */
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
    bool remove_from_buffer = true;
} scheduled_frame_t;

typedef struct
{
    packet_t packet;
    unsigned int transmission_power;
    unsigned int frequency;
} transmitted_frame_t;

typedef struct
{
    packet_t packet;
    unsigned int transmission_power;
    unsigned int frequency;
    double success_prob;
} received_frame_t;

typedef struct
{
    double snr_50_db;        /* beta: SNR for 50% success rate */
    double slope;            /* alpha: Steepness of sigmoid curve */
    double max_saturation;   /* gamma: The maximum possible transmission power when tx_power -> inf */
    double noise_floor_dbm;  /* Noise power in dBm */
} markov_state_t;

