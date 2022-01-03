#pragma once
#include "Types/CoreTypes.h"
#include "ProgramCoreExports.h"

class String;

// Data type to send time units in for conversions or other operations
using TimeConvType = float;
// Tick representation
using TickRep = int64;

// If real world clock time is needed use clockTimeNow
// If time interval is needed use timeNow

// IN MILLISECONDS PRECISION
namespace Time
{
    PROGRAMCORE_EXPORT TickRep addSeconds(const TickRep& tickValue, TimeConvType seconds);
    PROGRAMCORE_EXPORT TickRep addMinutes(const TickRep& tickValue, TimeConvType minutes);
    PROGRAMCORE_EXPORT TickRep addHours(const TickRep& tickValue, TimeConvType hours);
    PROGRAMCORE_EXPORT TickRep addDays(const TickRep& tickValue, TimeConvType days);

    PROGRAMCORE_EXPORT TimeConvType asSeconds(const TickRep& tickValue);
    PROGRAMCORE_EXPORT TimeConvType asMinutes(const TickRep& tickValue);
    PROGRAMCORE_EXPORT TimeConvType asHours(const TickRep& tickValue);
    PROGRAMCORE_EXPORT TimeConvType asDays(const TickRep& tickValue);

    PROGRAMCORE_EXPORT TickRep timeNow();
    PROGRAMCORE_EXPORT TickRep clockTimeNow();

    PROGRAMCORE_EXPORT TickRep fromPlatformTime(int64 platformTick);
}

// IN NANOSECONDS PRECISION
namespace HighResolutionTime
{
    PROGRAMCORE_EXPORT TickRep addSeconds(const TickRep& tickValue, TimeConvType seconds);
    PROGRAMCORE_EXPORT TickRep addMinutes(const TickRep& tickValue, TimeConvType minutes);
    PROGRAMCORE_EXPORT TickRep addHours(const TickRep& tickValue, TimeConvType hours);
    PROGRAMCORE_EXPORT TickRep addDays(const TickRep& tickValue, TimeConvType days);

    PROGRAMCORE_EXPORT TimeConvType asSeconds(const TickRep& tickValue);
    PROGRAMCORE_EXPORT TimeConvType asMinutes(const TickRep& tickValue);
    PROGRAMCORE_EXPORT TimeConvType asHours(const TickRep& tickValue);
    PROGRAMCORE_EXPORT TimeConvType asDays(const TickRep& tickValue);

    PROGRAMCORE_EXPORT TickRep timeNow();
    PROGRAMCORE_EXPORT TickRep clockTimeNow();

    PROGRAMCORE_EXPORT TickRep fromPlatformTime(int64 platformTick);
}

// Uses High Res Time
struct PROGRAMCORE_EXPORT StopWatch
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