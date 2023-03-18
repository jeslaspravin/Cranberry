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

#include <shared_mutex>

#if ENABLE_STRID_DEBUG

static std::shared_mutex stringDbLock;

std::unordered_map<StringID::IDType, String> &StringID::debugStringDB()
{
    static std::unordered_map<StringID::IDType, String> singletonStringDB;
    return singletonStringDB;
}

const TChar *StringID::findDebugString(IDType strId)
{
    std::shared_lock<std::shared_mutex> readLock{ stringDbLock };
    auto itr = debugStringDB().find(strId);
    if (itr == debugStringDB().cend())
    {
        return nullptr;
    }
    return itr->second.getChar();
}

void StringID::insertDbgStr(StringView str)
{
    std::unique_lock<std::shared_mutex> writeLock{ stringDbLock };
    debugStrings = &debugStringDB();
    debugStringDB().insert({ id, str });
}

#endif // DEV_BUILD

CONST_INIT const StringID StringID::INVALID = StringID(EInitType::InitType_DefaultInit);