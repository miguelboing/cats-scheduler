#pragma once

#include <vector>
#include <memory>

typedef struct
{
//    unsigned int period; /* T: Periodicy related with the task */
    unsigned int packet_id;       /* ID: Tasks unique identifier */
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
    std::vector<unsigned int> frame_allocation;  /* task ID per frame */
    bool is_feasible;                            /* whether all deadlines met */
    std::vector<packet_t> missed_deadlines;      /* tasks that missed deadlines */
}schedule_result_t;

//class PacketQueue {
//public:
//    void push_packet(std::shared_ptr<packet_t> packet);  // Parameter needed
//    std::shared_ptr<packet_t> pop_packet();             // Get packet from queue
//    bool empty() const;                                 // Check if empty
//    size_t size() const;                               // Get queue size
//
//private:
//    std::vector<std::shared_ptr<packet_t>> queue;
//};


