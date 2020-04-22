#pragma once
#include "../Platform/PlatformTypes.h"

//////////////////////////////////////////////////////////////////////////
//// EVERYTHING IS IN MILLISECONDS PRECISION
//////////////////////////////////////////////////////////////////////////

// Data type to send time units in for conversions or other operations
using TimeConvType = float;
// Tick representation
using TickRep = int64;

namespace Time
{
    TickRep addSeconds(const TickRep& tickValue, TimeConvType seconds);
    TickRep addMinutes(const TickRep& tickValue, TimeConvType minutes);
    TickRep addHours(const TickRep& tickValue, TimeConvType hours);
    TickRep addDays(const TickRep& tickValue, TimeConvType days);

    TimeConvType asSeconds(const TickRep& tickValue);
    TimeConvType asMinutes(const TickRep& tickValue);
    TimeConvType asHours(const TickRep& tickValue);
    TimeConvType asDays(const TickRep& tickValue);

    TickRep timeNow();
    TickRep clockTimeNow();
}