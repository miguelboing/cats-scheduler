#pragma once

#include <vector>
#include <memory>
#include <random>

typedef enum /* This enum defines how the results will be generated */
{
    BERNOULLI = 0,        /**< . */
    NORMAL,
    NO_DISTR,
} prob_distr_e;

class PHYChannel
{
public:
    PHYChannel(unsigned int number_of_frames, std::shared_ptr<std::vector<double>> channel_condition, prob_distr_e prob_distr);
    unsigned int number_of_frames; /* This is the size in frames of the simulation */
    prob_distr_e prob_distr;
    std::shared_ptr<std::vector<double>> channel_condition; /* This is the condition of the channel */
    void gen_frame_probabilities(void);

private:
    std::default_random_engine generator;
    template<typename Distribution>
        void fill_channel_condition(Distribution& distribution)
        {
            for (unsigned int i = 0U; i < this->number_of_frames; ++i)
            {
                double value = (double) distribution(this->generator);
                // Clamp value to [0, 1] for valid probability range
                value = std::max(0.0, std::min(1.0, value));
                (*this->channel_condition)[i] = value;
            };
        };
};

