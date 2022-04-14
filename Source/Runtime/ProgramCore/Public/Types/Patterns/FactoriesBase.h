/*!
 * \file FactoriesBase.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

template <typename TargetBase, typename... ConstructParams>
class FactoriesBase
{
public:
    virtual TargetBase create(ConstructParams... params) const = 0;
};