#pragma once
#include <vector>
#include <memory>
#include <random>

#include "system_model/system_model.hpp"

typedef enum /* This enum defines how the results will be generated */
{
    BERNOULLI = 0,        /**< . */
    NORMAL,
    NO_DISTR,
} prob_distr_e;

class PHYChannel
{
public:
    PHYChannel(system_model_t system_model, prob_distr_e prob_distr);
    void gen_frame_probabilities(prob_distr_e prob_distr);

    prob_distr_e prob_distr;
    system_model_t system_model;

private:
    std::default_random_engine generator;
    template<typename Distribution>
    void fill_channel_condition(std::vector<Distribution>& distributions) // cppcheck-suppress functionConst
    {
        /* Iterate through each channel */
        for (size_t ch_idx = 0; ch_idx < system_model.channels->size(); ++ch_idx)
        {
            auto& channel = (*system_model.channels)[ch_idx];

            /* For each power level */
            for (size_t pwr_idx = 0; pwr_idx < distributions.size(); ++pwr_idx)
            {
                /* For each frame */
                for (unsigned int frame = 0; frame < system_model.number_of_frames; ++frame)
                {
                    double value = (double) distributions[pwr_idx](this->generator);
                    value = std::max(0.0, std::min(1.0, value));
                    (*channel.channel_condition)[pwr_idx][frame] = value;
                }
            }
        }
    }
};

