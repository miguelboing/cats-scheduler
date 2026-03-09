#include <random>
#include <vector>
#include <algorithm>

#include "system_model/system_model.hpp"
#include "ml_predictor.hpp"

MLPredictor::MLPredictor(std::shared_ptr<unsigned int> sys_tick, std::shared_ptr<std::vector<SigmoidChannel>> sigmoid_channels): system_tick(sys_tick), channels(sigmoid_channels) {};

double MLPredictor::predict_channel_conditions(unsigned int frequency, unsigned int transmission_power)
{
    auto it = std::find_if(channels->begin(), channels->end(),
        [frequency](const SigmoidChannel& ch) {return ch.frequency == frequency;});

    if (it != channels->end())
    {
        return it->gen_probability(transmission_power);
    }

    /* -1 represents that the MLPredictor didn't recognize the frequency */
    return -1.0;
}

