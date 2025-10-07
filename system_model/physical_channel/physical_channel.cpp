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
    std::bernoulli_distribution distribution(0.0);

    switch(this->prob_distr)
    {
    case BERNOULLI: /* Initialize bernoulli distribution */
        distribution = std::bernoulli_distribution(0.5);

        break;
    default:
        distribution = std::bernoulli_distribution(0.0);

        break;
    }

    for (unsigned int i=0U; i < this->number_of_frames; ++i)
    {
        (*this->channel_condition)[i] = (double) distribution(this->generator);
    }
}

