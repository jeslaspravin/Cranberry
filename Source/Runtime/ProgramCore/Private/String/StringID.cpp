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

#include "Logger/Logger.h"

struct DebugStringIDsData
{
    std::shared_mutex lock;
    StringID::DebugStringsMap stringsDb;

    DebugStringIDsData() { StringID::debugStrings = &stringsDb; }
    MAKE_TYPE_NONCOPY_NONMOVE(DebugStringIDsData)
    ~DebugStringIDsData()
    {
        std::unique_lock<std::shared_mutex> writeLock{ lock };
        StringID::debugStrings = nullptr;
    }
};

DebugStringIDsData &debugStringDB()
{
    static DebugStringIDsData singletonStringDB;
    return singletonStringDB;
}

const TChar *StringID::findDebugString(IDType strId)
{
    DebugStringIDsData &stringsDbData = debugStringDB();

    std::shared_lock<std::shared_mutex> readLock{ stringsDbData.lock };
    auto itr = stringsDbData.stringsDb.find(strId);
    if (itr == stringsDbData.stringsDb.cend())
    {
        return nullptr;
    }
    if (itr->second.size() > 1)
    {
        LOG("StringID", "StringID {} has overlaps with values {}", strId, itr->second);
    }
    return itr->second.cbegin()->getChar();
}

void StringID::insertDbgStr(StringView str)
{
    if (str.empty())
    {
        return;
    }
    DebugStringIDsData &stringsDbData = debugStringDB();

    std::unique_lock<std::shared_mutex> writeLock{ stringsDbData.lock };
    stringsDbData.stringsDb[id].insert(str);
}

StringID::DebugStringsMap *StringID::debugStrings = nullptr;

#endif // DEV_BUILD

CONST_INIT const StringID StringID::INVALID = StringID(EInitType::InitType_DefaultInit);