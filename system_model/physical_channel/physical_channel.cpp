#include <vector>
#include <random>

#include "physical_channel.hpp"

PHYChannel::PHYChannel(system_model_t system_model, prob_distr_e prob_distr):
    system_model(system_model)
{
    this->gen_frame_probabilities(prob_distr);
}

void PHYChannel::gen_frame_probabilities(prob_distr_e prob_distr)
{
    this->prob_distr = prob_distr;

    switch(this->prob_distr)
    {
    case BERNOULLI: /* Initialize bernoulli distribution */
        {
            std::vector<std::bernoulli_distribution> distributions =
            {
                std::bernoulli_distribution(0.6),
                std::bernoulli_distribution(0.7),
                std::bernoulli_distribution(0.8)
            };

            this->fill_channel_condition(distributions);
        }
        break;
    case NORMAL:
        {
            std::vector<std::normal_distribution<double>> distributions =
            {
                std::normal_distribution<double>(0.6, 0.4),
                std::normal_distribution<double>(0.7, 0.4),
                std::normal_distribution<double>(0.8, 0.4)
            };

            this->fill_channel_condition(distributions);
        }
        break;
    default:
        {
            std::vector<std::bernoulli_distribution> distributions =
            {
                std::bernoulli_distribution(0.0),
                std::bernoulli_distribution(0.0),
                std::bernoulli_distribution(0.0)
            };

            this->fill_channel_condition(distributions);
        }
        break;
    }
}

