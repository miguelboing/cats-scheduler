#include <algorithm>
#include <iostream>

#include "system_model/system_model.hpp"

#include "transmitter.hpp"

Transmitter::Transmitter(std::shared_ptr<std::vector<packet_t>> buffer_packet):
    buffer_packet(buffer_packet) {};

transmitted_frame_t Transmitter::transmit_frame(scheduled_frame_t scheduled_frame)
{
    transmitted_frame_t transmitted_frame;
    transmitted_frame.transmission_power = scheduled_frame.transmission_power;
    transmitted_frame.frequency = scheduled_frame.frequency;

    if (scheduled_frame.packet == nullptr)
    {
        transmitted_frame.packet = {0, 0, 0, 0, 0, 0};

        return transmitted_frame;
    }

    transmitted_frame.packet = *(scheduled_frame.packet);

    /* Find packet on the buffer */
    auto packet_it = std::find_if(this->buffer_packet->begin(), this->buffer_packet->end(),
        [&scheduled_frame](const packet_t& p) { return &p == scheduled_frame.packet;});

    if (packet_it != this->buffer_packet->end())
    {
        if (scheduled_frame.packet->frames > scheduled_frame.packet->frame_count) /* Check if this packet is valid */
        {
            scheduled_frame.packet->frame_count++;
        }
    }
    else
    {
        std::cout << "ERROR: Couldn't find the packet" << std::endl;
    }

    return transmitted_frame;
}

