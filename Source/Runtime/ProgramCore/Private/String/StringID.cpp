/*!
 * \file StringID.cpp
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "String/StringID.h"

#if DEV_BUILD
std::unordered_map<StringID::IDType, String> &StringID::debugStringDB()
{
    static std::unordered_map<StringID::IDType, String> singletonStringDB;
    return singletonStringDB;
}
#endif // DEV_BUILD

CONST_INIT const StringID StringID::INVALID = StringID(EInitType::InitType_DefaultInit);