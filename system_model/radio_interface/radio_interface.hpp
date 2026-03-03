#pragma once

#include <optional>
#include <vector>
#include <memory>

#include "transmitter/transmitter.hpp"

#include "system_model/system_model.hpp"

class RadioInterface
{
public:
    explicit RadioInterface(std::shared_ptr<std::vector<packet_t>> buffer_packet);

    Transmitter transmitter;

    std::optional<transmitted_frame_t> transmit_frame(scheduled_frame_t scheduled_frame);
};

