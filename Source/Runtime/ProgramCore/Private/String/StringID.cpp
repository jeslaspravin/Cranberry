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

#if ENABLE_STRID_DEBUG

#include "Logger/Logger.h"

#include <unordered_map>
#include <unordered_set>
#include <shared_mutex>

using DebugStringsMap = std::unordered_map<StringID::IDType, std::unordered_set<String>>;

struct DebugStringIDsData
{
    std::shared_mutex lock;
    DebugStringsMap stringsDb;

    // Holds pointer to debugStringsDB which will be used by debug to visualize string
    static DebugStringsMap *debugStrings;

    DebugStringIDsData() { debugStrings = &stringsDb; }
    MAKE_TYPE_NONCOPY_NONMOVE(DebugStringIDsData)
    ~DebugStringIDsData()
    {
        std::unique_lock<std::shared_mutex> writeLock{ lock };
        debugStrings = nullptr;
    }
};
DebugStringsMap *DebugStringIDsData::debugStrings = nullptr;

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

#endif // DEV_BUILD

CONST_INIT const StringID StringID::INVALID = StringID(EInitType::InitType_DefaultInit);