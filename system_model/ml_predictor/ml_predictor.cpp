#include <random>
#include <vector>
#include <algorithm>

#include "system_model/system_model.hpp"
#include "ml_predictor.hpp"

MLPredictor::MLPredictor(std::shared_ptr<unsigned int> sys_tick, std::shared_ptr<std::vector<SigmoidChannel>> sigmoid_channels): system_tick(sys_tick), channels(sigmoid_channels) {};

std::vector<double> MLPredictor::predict_channel_conditions(unsigned int frequency, const std::vector<unsigned int>& powers)
{
    auto it = std::find_if(channels->begin(), channels->end(),
        [frequency](const SigmoidChannel& ch) {return ch.frequency == frequency;});

    if (it != channels->end())
    {
        std::vector<double> probs;
        for (unsigned int power : powers)
            probs.push_back(it->gen_probability(power));
        return probs;
    }

    /* -1 represents that the MLPredictor didn't recognize the frequency */
    return std::vector<double>(powers.size(), -1.0);
}

