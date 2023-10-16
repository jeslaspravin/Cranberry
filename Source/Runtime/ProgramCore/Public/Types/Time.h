/*!
 * \file Time.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "ProgramCoreExports.h"
#include "Types/CoreTypes.h"
#include "Types/CoreDefines.h"

class String;

// Data type to send time units in for conversions or other operations
using TimeConvType = float;
// Tick representation
using TickRep = int64;

// If real world clock time is needed use clockTimeNow
// If time interval is needed use timeNow

// IN MICROSECONDS PRECISION
namespace Time
{
PROGRAMCORE_EXPORT TickRep addSeconds(TickRep tickValue, TimeConvType seconds);
PROGRAMCORE_EXPORT TickRep addMinutes(TickRep tickValue, TimeConvType minutes);
PROGRAMCORE_EXPORT TickRep addHours(TickRep tickValue, TimeConvType hours);
PROGRAMCORE_EXPORT TickRep addDays(TickRep tickValue, TimeConvType days);

PROGRAMCORE_EXPORT TickRep fromMilliSeconds(TickRep milliSeconds);
FORCE_INLINE CONST_EXPR TickRep fromMicroSeconds(TickRep microSeconds) { return microSeconds; }
PROGRAMCORE_EXPORT TickRep fromNanoSeconds(TickRep nanoSeconds);

PROGRAMCORE_EXPORT TickRep fromSeconds(TimeConvType seconds);
PROGRAMCORE_EXPORT TickRep fromMinutes(TimeConvType minutes);
PROGRAMCORE_EXPORT TickRep fromHours(TimeConvType hours);
PROGRAMCORE_EXPORT TickRep fromDays(TimeConvType days);

PROGRAMCORE_EXPORT TickRep asMilliSeconds(TickRep tickValue);
FORCE_INLINE CONST_EXPR TickRep asMicroSeconds(TickRep tickValue) { return tickValue; }
PROGRAMCORE_EXPORT TickRep asNanoSeconds(TickRep tickValue);

PROGRAMCORE_EXPORT TimeConvType asSeconds(TickRep tickValue);
PROGRAMCORE_EXPORT TimeConvType asMinutes(TickRep tickValue);
PROGRAMCORE_EXPORT TimeConvType asHours(TickRep tickValue);
PROGRAMCORE_EXPORT TimeConvType asDays(TickRep tickValue);

PROGRAMCORE_EXPORT TickRep timeNow();
PROGRAMCORE_EXPORT TickRep clockTimeNow();
PROGRAMCORE_EXPORT TickRep utcTimeNow();
PROGRAMCORE_EXPORT TickRep localTimeNow();

PROGRAMCORE_EXPORT TickRep fromPlatformTime(int64 platformTick);
PROGRAMCORE_EXPORT int64 toPlatformTime(TickRep tickValue);

// Prints in format "dd-mm-yyyy HH:MM:SS.xxxxxx"
PROGRAMCORE_EXPORT String toString(TickRep tickValue, bool bIsUTC);
FORCE_INLINE uint32 toStringLen()
{
    return 11  /* "dd-mm-yyyy " */
           + 9 /* "HH:MM:SS." */
           + 6 /* "xxxxxx" */
        ;
}
} // namespace Time

// IN NANOSECONDS PRECISION
namespace HighResolutionTime
{
PROGRAMCORE_EXPORT TickRep addSeconds(TickRep tickValue, TimeConvType seconds);
PROGRAMCORE_EXPORT TickRep addMinutes(TickRep tickValue, TimeConvType minutes);
PROGRAMCORE_EXPORT TickRep addHours(TickRep tickValue, TimeConvType hours);
PROGRAMCORE_EXPORT TickRep addDays(TickRep tickValue, TimeConvType days);

PROGRAMCORE_EXPORT TickRep fromMilliSeconds(TickRep milliSeconds);
PROGRAMCORE_EXPORT TickRep fromMicroSeconds(TickRep microSeconds);
FORCE_INLINE CONST_EXPR TickRep fromNanoSeconds(TickRep nanoSeconds) { return nanoSeconds; }

PROGRAMCORE_EXPORT TickRep fromSeconds(TimeConvType seconds);
PROGRAMCORE_EXPORT TickRep fromMinutes(TimeConvType minutes);
PROGRAMCORE_EXPORT TickRep fromHours(TimeConvType hours);
PROGRAMCORE_EXPORT TickRep fromDays(TimeConvType days);

PROGRAMCORE_EXPORT TickRep asMilliSeconds(TickRep tickValue);
PROGRAMCORE_EXPORT TickRep asMicroSeconds(TickRep tickValue);
FORCE_INLINE CONST_EXPR TickRep asNanoSeconds(TickRep tickValue) { return tickValue; }

PROGRAMCORE_EXPORT TimeConvType asSeconds(TickRep tickValue);
PROGRAMCORE_EXPORT TimeConvType asMinutes(TickRep tickValue);
PROGRAMCORE_EXPORT TimeConvType asHours(TickRep tickValue);
PROGRAMCORE_EXPORT TimeConvType asDays(TickRep tickValue);

PROGRAMCORE_EXPORT TickRep timeNow();
PROGRAMCORE_EXPORT TickRep clockTimeNow();
PROGRAMCORE_EXPORT TickRep utcTimeNow();
PROGRAMCORE_EXPORT TickRep localTimeNow();

PROGRAMCORE_EXPORT TickRep fromPlatformTime(int64 platformTick);
PROGRAMCORE_EXPORT int64 toPlatformTime(TickRep tickValue);

// Prints in format "dd-mm-yyyy HH:MM:SS.xxxxxxxxx"
PROGRAMCORE_EXPORT String toString(TickRep tickValue, bool bIsUTC);
FORCE_INLINE uint32 toStringLen()
{
    return 11  /* "dd-mm-yyyy " */
           + 9 /* "HH:MM:SS." */
           + 9 /* "xxxxxxxxx" */
        ;
}
} // namespace HighResolutionTime

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