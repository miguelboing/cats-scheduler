# CATS Scheduler

A C++ simulator for wireless communication scheduling algorithms with real-time packet deadline management and channel condition modeling.

## Overview

CATS Scheduler simulates a wireless communication system where packets with strict deadlines must be transmitted over channels with varying conditions. The simulator models the complete transmission pipeline from packet generation through scheduling, transmission, channel propagation, and reception.

## Features

- **Packet Generation**: Configurable packet generators with customizable arrival patterns
  - Fixed-rate packet generation
  - Extensible base class for custom generators

- **Scheduling Algorithms**: Real-time scheduling with deadline awareness
  - Earliest Deadline First (EDF) scheduler
  - Extensible architecture for additional scheduling policies

- **Channel Modeling**: Realistic wireless channel simulation
  - Sigmoid-based probability model
  - Configurable SNR, noise, and path loss parameters
  - Multiple frequency support

- **System Components**:
  - Packet buffer with automatic deadline checking
  - Transmitter with power control
  - Receiver with success probability calculation
  - JSON-based logging for analysis

## Architecture

### Core Components

- **BufferPacket**: Manages pending packets and enforces deadline constraints
- **PacketGenerator**: Creates packets according to specified traffic patterns
- **Scheduler**: Selects packets for transmission based on scheduling policy
- **Transmitter**: Prepares frames for transmission with appropriate power levels
- **SigmoidChannel**: Models channel conditions and calculates reception probability
- **Receiver**: Decodes received frames and logs results

### Data Flow

1. Packet generators create packets and add them to the buffer
2. Scheduler selects packets based on deadlines and other criteria
3. Transmitter prepares the selected packet for transmission
4. Physical channel applies propagation effects and calculates success probability
5. Receiver attempts to decode the frame based on channel conditions
6. Buffer removes packets that miss their deadlines

## Code Quality

The codebase is **cppcheck compliant** with the following checks enabled:
- Warning detection
- Style checks
- Performance analysis
- Portability verification

## Building

### Prerequisites

- C++ compiler with C++11 support (g++ recommended)
- Make
- [nlohmann/json](https://github.com/nlohmann/json) library (included in `libs/`)

### Compilation

```bash
make
```

This will compile all components and create the executable `main.o`.

### Clean Build

```bash
make clean
make
```

## Usage

### Basic Example

```cpp
#include "system_model/system_model.hpp"
#include "packet_generators/fixed_rate/fixed_rate.hpp"
#include "schedulers/earliest_deadline_first/edf_scheduler.hpp"
#include "physical_channels/sigmoid_channel/sigmoid_channel.hpp"

// Initialize system components
std::shared_ptr<unsigned int> system_tick = std::make_shared<unsigned int>(0U);
BufferPacket buffer(system_tick);

// Configure packet generators
std::vector<fixed_rate_packet_t> packets;
packets.push_back(fixed_rate_packet_t(5U, 2U, 0.9, 1U, 5U, 0U));
// Parameters: period, frames, reliability, id, deadline, offset

FixedRate_PacketGen packet_gen(system_tick, packets, buffer.buffer_packet, spawn_log);

// Initialize scheduler and transmitter
EDF_scheduler scheduler(12, 14074000, buffer.buffer_packet);
Transmitter transmitter(buffer.buffer_packet);

// Initialize channel
SigmoidChannel channel(14074000);

// Initialize receiver
Receiver receiver(system_tick);

// Run simulation
for (unsigned int i = 0; i < num_frames; i++) {
    packet_gen.generate_packets();
    auto scheduled = scheduler.schedule_frame();
    auto transmitted = transmitter.transmit_frame(scheduled);
    auto received = channel.gen_frame_with_probability(transmitted);
    receiver.recv_frame(received);
    buffer.check_deadlines();
    (*system_tick)++;
}

// Save results
receiver.save_to_file("receiver_results.json");
```

### Running the Example

```bash
./main.o
```

This runs the default simulation from `main.cpp` and generates:
- `generated_packets.json`: Log of all generated packets
- `received_packets.json`: Log of all received packets with success/failure status

## Configuration

### Packet Generator Parameters

Fixed-rate packets are configured with:
- **Period**: Frames between packet arrivals
- **Frames**: Number of frames required to transmit the packet
- **Reliability**: Required probability of successful reception (0.0-1.0)
- **ID**: Unique identifier for the packet flow
- **Deadline**: Relative deadline in frames
- **Offset**: Initial delay before first packet

### Scheduler Parameters

EDF scheduler requires:
- **Subframe size**: Number of symbols per frame
- **Frequency**: Operating frequency in Hz
- **Buffer**: Reference to the packet buffer

### Channel Parameters

Sigmoid channel supports:
- **Frequency**: Operating frequency
- **SNR50**: SNR value at 50% success probability
- **Slope**: Steepness of the sigmoid curve
- **Noise floor**: Receiver noise level in dBm
- **Path loss**: Channel path loss in dB

## Project Structure

```
.
├── main.cpp                          # Example simulation
├── Makefile                          # Build configuration
├── system_model/                     # Core data structures
│   ├── buffer_packet/                # Packet buffer management
│   ├── transmitter/                  # Frame transmission
│   └── receiver/                     # Frame reception
├── packet_generators/                # Traffic generation
│   └── fixed_rate/                   # Fixed-rate generator
├── schedulers/                       # Scheduling algorithms
│   └── earliest_deadline_first/      # EDF implementation
├── physical_channels/                # Channel models
│   └── sigmoid_channel/              # Sigmoid probability model
└── libs/                             # External libraries
    └── nlohmann/                     # JSON library
```

## Extending the Simulator

### Adding a New Scheduler

1. Inherit from `BaseScheduler`
2. Implement `schedule_frame()` method
3. Add Makefile in `schedulers/<your_scheduler>/`
4. Update main Makefile

### Adding a New Packet Generator

1. Inherit from `BasePacketGenerator`
2. Implement `generate_packets()` method
3. Add Makefile in `packet_generators/<your_generator>/`
4. Update main Makefile

### Adding a New Channel Model

1. Inherit from `BasePhysicalChannel`
2. Implement `gen_frame_with_probability()` method
3. Add Makefile in `physical_channels/<your_channel>/`
4. Update main Makefile

## Output Format

### Generated Packets Log
```json
[
  {
    "system_tick": 0,
    "packet_id": 1,
    "packet_id_count": 0,
    "deadline": 5,
    "frames": 2,
    "reliability_req": 0.9,
    "generator_type": "FixedRate"
  }
]
```

### Received Packets Log
```json
[
  {
    "system_tick": 1,
    "packet_id": 1,
    "packet_id_count": 0,
    "frame_count": 1,
    "success_prob": 0.95,
    "decoded": true
  }
]
```

## Credits

This project uses the following open-source libraries:

- **[nlohmann/json](https://github.com/nlohmann/json)** - JSON for Modern C++ by Niels Lohmann (MIT License)

## License

See [LICENSE](LICENSE) file for details.

## Contributing

Contributions are welcome! Please ensure:
- Code follows existing style conventions
- New components include appropriate Makefiles
- Complex features include usage examples
