#pragma once

#include "system_model/system_model.hpp"

class Transmitter
{
public:
    std::shared_ptr<std::vector<packet_t>> buffer_packet;

    explicit Transmitter(std::shared_ptr<std::vector<packet_t>> buffer_packet);

    std::optional<transmitted_frame_t> transmit_frame(scheduled_frame_t scheduled_frame);
};
