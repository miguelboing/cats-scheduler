#include <algorithm>
#include <iostream>

#include "system_model/system_model.hpp"

#include "transmitter.hpp"

transmitted_packet_t Transmitter::transmit_frame(scheduled_packet_t scheduled_packet)
{
    transmitted_packet_t transmitted_packet;
    transmitted_packet.transmission_power = scheduled_packet.transmission_power;
    transmitted_packet.frequency = scheduled_packet.frequency;

    if (scheduled_packet.packet == nullptr)
    {
        transmitted_packet.packet = {0, 0, 0, 0, 0, 0};

        return transmitted_packet;
    }

    transmitted_packet.packet = *(scheduled_packet.packet);

    /* Find packet on the buffer */
    auto packet_it = std::find_if(this->buffer_packet->begin(), this->buffer_packet->end(),
        [&scheduled_packet](const packet_t& p) { return &p == scheduled_packet.packet;});

    if (packet_it != this->buffer_packet->end())
    {
        if (scheduled_packet.packet->frames > scheduled_packet.packet->frame_count) /* Check if this packet is valid */
        {
            scheduled_packet.packet->frame_count++;
        }
    }
    else
    {
        std::cout << "ERROR: Couldn't find the packet" << std::endl;
    }

    return transmitted_packet;
}

