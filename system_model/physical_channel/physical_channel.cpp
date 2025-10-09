#include <vector>
#include <random>

#include "physical_channel.hpp"

PHYChannel::PHYChannel(unsigned int number_of_frames, std::shared_ptr<std::vector<double>> channel_condition, prob_distr_e prob_distr):
        number_of_frames(number_of_frames), prob_distr(prob_distr)
{
    this->channel_condition = channel_condition;  // Use the passed shared_ptr
    this->gen_frame_probabilities();
}

void PHYChannel::gen_frame_probabilities()
{
    switch(this->prob_distr)
    {
    case BERNOULLI: /* Initialize bernoulli distribution */
        {
            std::bernoulli_distribution distribution(0.5);
            this->fill_channel_condition(distribution);
        }
        break;
    case NORMAL:
        {
            std::normal_distribution distribution(0.8, 0.4);
            this->fill_channel_condition(distribution);
        }
        break;
    default:
        {
            std::bernoulli_distribution distribution(0.0);
            this->fill_channel_condition(distribution);
        }
        break;
    }
}

