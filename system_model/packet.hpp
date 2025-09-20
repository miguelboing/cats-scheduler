#ifndef PACKETS_H_
#define PACKETS_H_

typedef struct
{
	unsigned int period; /* T:  */
	unsigned int deadline; /* D: This is the relative deadline */
	unsigned int comp_cost; /* C: A unit consumes one frame */
	unsigned int success_rate; /* S: Sureness that this packet was received */
} packet_t;

typedef struct
{
	unsigned int number_of_frames; /* This is the total number of frames available to transmit */
	float channel_condition[number_of_frames][3]; /* Channel conditions for each frame at different power levels */
} system_model_t;

#endif /* PACKETS_H_ */
