#pragma once
#include "../Platform/PlatformTypes.h"

// Data type to send time units in for conversions or other operations
using TimeConvType = float;
// Tick representation
using TickRep = int64;

// IN MILLISECONDS PRECISION
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

// IN NANOSECONDS PRECISION
namespace HighResolutionTime
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

// Uses High Res Time
struct StopWatch
{
private:
    TickRep startTime = 0;
    TickRep lastLapTime = 0;
    TickRep stopTime = 0;
public:
    StopWatch(bool bStart = true);
    ~StopWatch() = default;

    TickRep start();
    TickRep stop();
    TickRep lap();

    // In seconds
    TimeConvType lapTime() const;
    // Time since last lap and now, if no last lap then start
    TimeConvType thisLap() const;
    // Returns duration btw stop time and start time if stopped else btw current time and start
    TimeConvType duration() const;
};