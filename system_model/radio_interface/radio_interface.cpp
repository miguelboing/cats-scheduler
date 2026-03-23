#include <optional>
#include <vector>
#include <memory>

#include "system_model/system_model.hpp"

#include "transmitter/transmitter.hpp"

#include "radio_interface.hpp"

RadioInterface::RadioInterface(std::shared_ptr<std::vector<packet_t>> buffer_packet):
    transmitter(buffer_packet) {};

std::optional<transmitted_frame_t> RadioInterface::transmit_frame(scheduled_frame_t scheduled_frame)
{
    return transmitter.transmit_frame(scheduled_frame);
}

