#include "Math.h"

#include <random>

std::random_device Math::rDevice;

float Math::random()
{
    std::default_random_engine generator(rDevice());
    std::uniform_real_distribution<float> distribution(0.0, 1.0);

    return distribution(generator);
}

