/*!
 * \file MathInternal.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Math/Math.h"

#include <random>

std::random_device Math::rDevice;

float Math::random()
{
    // Use SFMT if MT is not fast enough in furture
    // http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/SFMT/index.html
    std::default_random_engine generator(rDevice());
    std::uniform_real_distribution<float> distribution(0.0, 1.0);

    return distribution(generator);
}
