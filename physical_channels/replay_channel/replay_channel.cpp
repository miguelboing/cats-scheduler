#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "replay_channel.hpp"

namespace
{
    /* Fixed predictor view of the channel — what CATS sees in RX mode. */
    constexpr double PROB_1W  = 0.07;
    constexpr double PROB_10W = 0.58;
    constexpr double PROB_25W = 0.80;
}

ReplayChannel::ReplayChannel(unsigned int frequency,
                             const std::string& csv_path,
                             std::shared_ptr<unsigned int> sys_tick):
    BasePhysicalChannel(frequency), system_tick(sys_tick)
{
    std::ifstream f(csv_path);
    if (!f.is_open())
        throw std::runtime_error("ReplayChannel: could not open CSV: " + csv_path);

    std::string line;
    /* Skip header */
    if (!std::getline(f, line))
        throw std::runtime_error("ReplayChannel: empty CSV: " + csv_path);

    while (std::getline(f, line))
    {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string cell;

        /* Column 0: timeframe (ignored — implicit by row order) */
        if (!std::getline(ss, cell, ',')) continue;

        std::array<int, 3> succ{};
        for (int i = 0; i < 3; ++i)
        {
            if (!std::getline(ss, cell, ','))
                throw std::runtime_error("ReplayChannel: malformed row in " + csv_path);
            succ[i] = std::stoi(cell);
        }
        /* Remaining columns (maxdist_*) ignored. */
        rows.push_back(succ);
    }

    if (rows.empty())
        throw std::runtime_error("ReplayChannel: no data rows in " + csv_path);
}

double ReplayChannel::gen_probability(unsigned int transmission_power)
{
    switch (transmission_power)
    {
        case 1U:  return PROB_1W;
        case 10U: return PROB_10W;
        case 25U: return PROB_25W;
        default:
            std::cerr << "ReplayChannel: unsupported power " << transmission_power
                      << "W — returning 0.0\n";
            return 0.0;
    }
}

received_frame_t ReplayChannel::gen_frame_with_probability(transmitted_frame_t transmitted_frame)
{
    received_frame_t recv;
    recv.packet             = transmitted_frame.packet;
    recv.transmission_power = transmitted_frame.transmission_power;
    recv.frequency          = transmitted_frame.frequency;

    int col;
    switch (transmitted_frame.transmission_power)
    {
        case 1U:  col = 0; break;
        case 10U: col = 1; break;
        case 25U: col = 2; break;
        default:
            std::cerr << "ReplayChannel: unsupported power "
                      << transmitted_frame.transmission_power
                      << "W — treating frame as failed\n";
            recv.success_prob = 0.0;
            return recv;
    }

    const unsigned int t = *system_tick;
    if (t >= rows.size())
    {
        /* Past the recorded window — treat as failed rather than wrapping
           around, which would silently corrupt long-duration runs. */
        recv.success_prob = 0.0;
        return recv;
    }

    recv.success_prob = (rows[t][col] != 0) ? 1.0 : 0.0;
    return recv;
}

