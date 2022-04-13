/*!
 * \file Time.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

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
    PROGRAMCORE_EXPORT TickRep addSeconds(TickRep tickValue, TimeConvType seconds);
    PROGRAMCORE_EXPORT TickRep addMinutes(TickRep tickValue, TimeConvType minutes);
    PROGRAMCORE_EXPORT TickRep addHours(TickRep tickValue, TimeConvType hours);
    PROGRAMCORE_EXPORT TickRep addDays(TickRep tickValue, TimeConvType days);

    PROGRAMCORE_EXPORT TickRep fromSeconds(TimeConvType seconds);
    PROGRAMCORE_EXPORT TickRep fromMinutes(TimeConvType minutes);
    PROGRAMCORE_EXPORT TickRep fromHours(TimeConvType hours);
    PROGRAMCORE_EXPORT TickRep fromDays(TimeConvType days);

    PROGRAMCORE_EXPORT TimeConvType asSeconds(TickRep tickValue);
    PROGRAMCORE_EXPORT TimeConvType asMinutes(TickRep tickValue);
    PROGRAMCORE_EXPORT TimeConvType asHours(TickRep tickValue);
    PROGRAMCORE_EXPORT TimeConvType asDays(TickRep tickValue);

    PROGRAMCORE_EXPORT TickRep timeNow();
    PROGRAMCORE_EXPORT TickRep clockTimeNow();

    PROGRAMCORE_EXPORT TickRep fromPlatformTime(int64 platformTick);
    PROGRAMCORE_EXPORT int64 toPlatformTime(TickRep tickValue);
}

// IN NANOSECONDS PRECISION
namespace HighResolutionTime
{
    PROGRAMCORE_EXPORT TickRep addSeconds(TickRep tickValue, TimeConvType seconds);
    PROGRAMCORE_EXPORT TickRep addMinutes(TickRep tickValue, TimeConvType minutes);
    PROGRAMCORE_EXPORT TickRep addHours(TickRep tickValue, TimeConvType hours);
    PROGRAMCORE_EXPORT TickRep addDays(TickRep tickValue, TimeConvType days);

    PROGRAMCORE_EXPORT TickRep fromSeconds(TimeConvType seconds);
    PROGRAMCORE_EXPORT TickRep fromMinutes(TimeConvType minutes);
    PROGRAMCORE_EXPORT TickRep fromHours(TimeConvType hours);
    PROGRAMCORE_EXPORT TickRep fromDays(TimeConvType days);

    PROGRAMCORE_EXPORT TimeConvType asSeconds(TickRep tickValue);
    PROGRAMCORE_EXPORT TimeConvType asMinutes(TickRep tickValue);
    PROGRAMCORE_EXPORT TimeConvType asHours(TickRep tickValue);
    PROGRAMCORE_EXPORT TimeConvType asDays(TickRep tickValue);

    PROGRAMCORE_EXPORT TickRep timeNow();
    PROGRAMCORE_EXPORT TickRep clockTimeNow();

    PROGRAMCORE_EXPORT TickRep fromPlatformTime(int64 platformTick);
    PROGRAMCORE_EXPORT int64 toPlatformTime(TickRep tickValue);
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

    // last lap tick from start
    TickRep lapTick() const;
    // This lap tick from last lap
    TickRep thisLapTick() const;
    // total ticks from start to stop or now
    TickRep durationTick() const;
    // In seconds
    TimeConvType lapTime() const;
    // Time since last lap and now, if no last lap then start
    TimeConvType thisLap() const;
    // Returns duration btw stop time and start time if stopped else btw current time and start
    TimeConvType duration() const;
};