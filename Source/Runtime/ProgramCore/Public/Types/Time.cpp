/*!
 * \file Time.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Types/Time.h"
#include "String/String.h"

#include <chrono>
#include <format>
#include <ratio>

// Will be implemented in platform specific TU
template <typename Resolution>
TickRep fromPlatformTime(int64 platformTick);
template <typename Resolution>
int64 toPlatformTime(TickRep tickValue);

using TimeResolution = std::chrono::microseconds;
using TimeHighResolution = std::chrono::nanoseconds;

template <bool bIsHighRes = false>
class TimeHelper
{
public:
    using DurationResolution = std::conditional_t<bIsHighRes, TimeHighResolution, TimeResolution>;
    using UTCTimePoint = std::chrono::time_point<std::chrono::utc_clock, DurationResolution>;
    using SystemTimePoint = std::chrono::time_point<std::chrono::system_clock, DurationResolution>;

public:
    FORCE_INLINE static TickRep timeNow()
    {
        using namespace std::chrono;
        return duration_cast<DurationResolution>(steady_clock::now().time_since_epoch()).count();
    }

    FORCE_INLINE static TickRep clockTimeNow()
    {
        using namespace std::chrono;
        return duration_cast<DurationResolution>(system_clock::now().time_since_epoch()).count();
    }
    FORCE_INLINE static TickRep utcTimeNow()
    {
        using namespace std::chrono;
        return duration_cast<DurationResolution>(utc_clock::now().time_since_epoch()).count();
    }

    FORCE_INLINE static TickRep localTimeNow()
    {
        using namespace std::chrono;
        static const time_zone *sessionZone = current_zone();
        return duration_cast<DurationResolution>(sessionZone->to_local(system_clock::now()).time_since_epoch()).count();
    }

    FORCE_INLINE static TickRep asMilliSeconds(TickRep tickValue)
    {
        using namespace std::chrono;
        return duration_cast<duration<TickRep, std::milli>>(DurationResolution(tickValue)).count();
    }
    FORCE_INLINE static TickRep asMicroSeconds(TickRep tickValue)
    {
        using namespace std::chrono;
        return duration_cast<duration<TickRep, std::micro>>(DurationResolution(tickValue)).count();
    }
    FORCE_INLINE static TickRep asNanoSeconds(TickRep tickValue)
    {
        using namespace std::chrono;
        return duration_cast<duration<TickRep, std::nano>>(DurationResolution(tickValue)).count();
    }

    FORCE_INLINE static TimeConvType asSeconds(TickRep tickValue)
    {
        using namespace std::chrono;
        return duration_cast<duration<TimeConvType>>(DurationResolution(tickValue)).count();
    }

    FORCE_INLINE static TimeConvType asMinutes(TickRep tickValue)
    {
        using namespace std::chrono;
        return duration_cast<duration<TimeConvType, std::ratio<60>>>(DurationResolution(tickValue)).count();
    }

    FORCE_INLINE static TimeConvType asHours(TickRep tickValue)
    {
        using namespace std::chrono;
        return duration_cast<duration<TimeConvType, std::ratio<3600>>>(DurationResolution(tickValue)).count();
    }

    FORCE_INLINE static TimeConvType asDays(TickRep tickValue)
    {
        using namespace std::chrono;
        return duration_cast<duration<TimeConvType, std::ratio<86400>>>(DurationResolution(tickValue)).count();
    }

    // FORCE_INLINE static String asString(TickRep tickValue, const String& formatStr)
    //{
    //     using namespace std::chrono;
    //     steady_clock::time_point tickTimePoint(Resolution(tickValue));
    //     std::tm timeBuffer;
    //
    //     std::stringstream outStr;
    //     outStr << put_time(localtime_s(&tickTimePoint, &timeBuffer), formatStr.getChar());
    //     return outStr.str();
    // }

    FORCE_INLINE static TickRep addSeconds(TickRep tickValue, TimeConvType seconds)
    {
        using namespace std::chrono;
        duration<TimeConvType> tickDur(asSeconds(tickValue));
        duration<TimeConvType> secondsToAdd(seconds);
        tickDur += secondsToAdd;
        return duration_cast<DurationResolution>(tickDur).count();
    }

    FORCE_INLINE static TickRep addMinutes(TickRep tickValue, TimeConvType minutes)
    {
        using namespace std::chrono;
        duration<TimeConvType, std::ratio<60>> tickDur(asMinutes(tickValue));
        duration<TimeConvType, std::ratio<60>> minutesToAdd(minutes);
        tickDur += minutesToAdd;
        return duration_cast<DurationResolution>(tickDur).count();
    }

    FORCE_INLINE static TickRep addHours(TickRep tickValue, TimeConvType hours)
    {
        using namespace std::chrono;
        duration<TimeConvType, std::ratio<3600>> tickDur(asHours(tickValue));
        duration<TimeConvType, std::ratio<3600>> hoursToAdd(hours);
        tickDur += hoursToAdd;
        return duration_cast<DurationResolution>(tickDur).count();
    }

    FORCE_INLINE static TickRep addDays(TickRep tickValue, TimeConvType days)
    {
        using namespace std::chrono;
        duration<TimeConvType, std::ratio<86400>> tickDur(asDays(tickValue));
        duration<TimeConvType, std::ratio<86400>> daysToAdd(days);
        tickDur += daysToAdd;
        return duration_cast<DurationResolution>(tickDur).count();
    }

    FORCE_INLINE static TickRep fromMilliSeconds(TickRep milliSeconds)
    {
        using namespace std::chrono;
        return duration_cast<DurationResolution>(duration<TickRep, std::milli>(milliSeconds)).count();
    }
    FORCE_INLINE static TickRep fromMicroSeconds(TickRep microSeconds)
    {
        using namespace std::chrono;
        return duration_cast<DurationResolution>(duration<TickRep, std::micro>(microSeconds)).count();
    }
    FORCE_INLINE static TickRep fromNanoSeconds(TickRep nanoSeconds)
    {
        using namespace std::chrono;
        return duration_cast<DurationResolution>(duration<TickRep, std::nano>(nanoSeconds)).count();
    }

    FORCE_INLINE static TickRep fromSeconds(TimeConvType seconds)
    {
        using namespace std::chrono;
        duration<TimeConvType> secondsToAdd(seconds);
        return duration_cast<DurationResolution>(secondsToAdd).count();
    }

    FORCE_INLINE static TickRep fromMinutes(TimeConvType minutes)
    {
        using namespace std::chrono;
        duration<TimeConvType, std::ratio<60>> minsToAdd(minutes);
        return duration_cast<DurationResolution>(minsToAdd).count();
    }

    FORCE_INLINE static TickRep fromHours(TimeConvType hours)
    {
        using namespace std::chrono;
        duration<TimeConvType, std::ratio<3600>> hoursToAdd(hours);
        return duration_cast<DurationResolution>(hoursToAdd).count();
    }

    FORCE_INLINE static TickRep fromDays(TimeConvType days)
    {
        using namespace std::chrono;
        duration<TimeConvType, std::ratio<86400>> daysToAdd(days);
        return duration_cast<DurationResolution>(daysToAdd).count();
    }

    FORCE_INLINE static TickRep fromPlatformTime(int64 platformTick) { return ::fromPlatformTime<DurationResolution>(platformTick); }

    FORCE_INLINE static int64 toPlatformTime(TickRep tickValue) { return ::toPlatformTime<DurationResolution>(tickValue); }

    FORCE_INLINE static String toString(TickRep tickValue, bool bIsUTC)
    {
        constexpr static const TChar *FORMAT_STR = TCHAR("{0:%d-%m-%Y %H:%M:%S}");
        if (bIsUTC)
        {
            return std::format(FORMAT_STR, UTCTimePoint(DurationResolution(tickValue)));
        }
        return std::format(FORMAT_STR, SystemTimePoint(DurationResolution(tickValue)));
    }
};

// namespace Time

TickRep Time::timeNow() { return TimeHelper<false>::timeNow(); }

TickRep Time::clockTimeNow() { return TimeHelper<false>::clockTimeNow(); }

TickRep Time::utcTimeNow() { return TimeHelper<false>::utcTimeNow(); }

TickRep Time::localTimeNow() { return TimeHelper<false>::localTimeNow(); }

TickRep Time::asNanoSeconds(TickRep tickValue) { return TimeHelper<false>::asNanoSeconds(tickValue); }

TickRep Time::asMilliSeconds(TickRep tickValue) { return TimeHelper<false>::asMilliSeconds(tickValue); }

TimeConvType Time::asSeconds(TickRep tickValue) { return TimeHelper<false>::asSeconds(tickValue); }

TimeConvType Time::asMinutes(TickRep tickValue) { return TimeHelper<false>::asMinutes(tickValue); }

TimeConvType Time::asHours(TickRep tickValue) { return TimeHelper<false>::asHours(tickValue); }

TimeConvType Time::asDays(TickRep tickValue) { return TimeHelper<false>::asDays(tickValue); }

TickRep Time::addSeconds(TickRep tickValue, TimeConvType seconds) { return TimeHelper<false>::addSeconds(tickValue, seconds); }

TickRep Time::addMinutes(TickRep tickValue, TimeConvType minutes) { return TimeHelper<false>::addMinutes(tickValue, minutes); }

TickRep Time::addHours(TickRep tickValue, TimeConvType hours) { return TimeHelper<false>::addHours(tickValue, hours); }

TickRep Time::addDays(TickRep tickValue, TimeConvType days) { return TimeHelper<false>::addDays(tickValue, days); }

TickRep Time::fromMilliSeconds(TickRep milliSeconds) { return TimeHelper<false>::fromMilliSeconds(milliSeconds); }

TickRep Time::fromNanoSeconds(TickRep nanoSeconds) { return TimeHelper<false>::fromNanoSeconds(nanoSeconds); }

TickRep Time::fromSeconds(TimeConvType seconds) { return TimeHelper<false>::fromSeconds(seconds); }

TickRep Time::fromMinutes(TimeConvType minutes) { return TimeHelper<false>::fromMinutes(minutes); }

TickRep Time::fromHours(TimeConvType hours) { return TimeHelper<false>::fromHours(hours); }

TickRep Time::fromDays(TimeConvType days) { return TimeHelper<false>::fromDays(days); }

TickRep Time::fromPlatformTime(int64 platformTick) { return TimeHelper<false>::fromPlatformTime(platformTick); }

int64 Time::toPlatformTime(TickRep tickValue) { return TimeHelper<false>::toPlatformTime(tickValue); }

String Time::toString(TickRep tickValue, bool bIsUTC) { return TimeHelper<false>::toString(tickValue, bIsUTC); }

// namespace HighResolutionTime

TickRep HighResolutionTime::timeNow() { return TimeHelper<true>::timeNow(); }

TickRep HighResolutionTime::clockTimeNow() { return TimeHelper<true>::clockTimeNow(); }

TickRep HighResolutionTime::utcTimeNow() { return TimeHelper<true>::utcTimeNow(); }

TickRep HighResolutionTime::localTimeNow() { return TimeHelper<true>::localTimeNow(); }

TickRep HighResolutionTime::asMilliSeconds(TickRep tickValue) { return TimeHelper<true>::asMilliSeconds(tickValue); }

TickRep HighResolutionTime::asMicroSeconds(TickRep tickValue) { return TimeHelper<true>::asMicroSeconds(tickValue); }

TimeConvType HighResolutionTime::asSeconds(TickRep tickValue) { return TimeHelper<true>::asSeconds(tickValue); }

TimeConvType HighResolutionTime::asMinutes(TickRep tickValue) { return TimeHelper<true>::asMinutes(tickValue); }

TimeConvType HighResolutionTime::asHours(TickRep tickValue) { return TimeHelper<true>::asHours(tickValue); }

TimeConvType HighResolutionTime::asDays(TickRep tickValue) { return TimeHelper<true>::asDays(tickValue); }

TickRep HighResolutionTime::addSeconds(TickRep tickValue, TimeConvType seconds) { return TimeHelper<true>::addSeconds(tickValue, seconds); }

TickRep HighResolutionTime::addMinutes(TickRep tickValue, TimeConvType minutes) { return TimeHelper<true>::addMinutes(tickValue, minutes); }

TickRep HighResolutionTime::addHours(TickRep tickValue, TimeConvType hours) { return TimeHelper<true>::addHours(tickValue, hours); }

TickRep HighResolutionTime::fromMilliSeconds(TickRep milliSeconds) { return TimeHelper<true>::fromMilliSeconds(milliSeconds); }

TickRep HighResolutionTime::fromMicroSeconds(TickRep microSeconds) { return TimeHelper<true>::fromMicroSeconds(microSeconds); }

TickRep HighResolutionTime::fromSeconds(TimeConvType seconds) { return TimeHelper<true>::fromSeconds(seconds); }

TickRep HighResolutionTime::fromMinutes(TimeConvType minutes) { return TimeHelper<true>::fromMinutes(minutes); }

TickRep HighResolutionTime::fromHours(TimeConvType hours) { return TimeHelper<true>::fromHours(hours); }

TickRep HighResolutionTime::fromDays(TimeConvType days) { return TimeHelper<true>::fromDays(days); }

TickRep HighResolutionTime::addDays(TickRep tickValue, TimeConvType days) { return TimeHelper<true>::addDays(tickValue, days); }

TickRep HighResolutionTime::fromPlatformTime(int64 platformTick) { return TimeHelper<true>::fromPlatformTime(platformTick); }

int64 HighResolutionTime::toPlatformTime(TickRep tickValue) { return TimeHelper<true>::toPlatformTime(tickValue); }

String HighResolutionTime::toString(TickRep tickValue, bool bIsUTC) { return TimeHelper<true>::toString(tickValue, bIsUTC); }

// Stopwatch

StopWatch::StopWatch(bool bStart /*= true*/)
{
    if (bStart)
    {
        startTime = HighResolutionTime::timeNow();
    }
}

TickRep StopWatch::start() { return startTime == 0 ? (startTime = HighResolutionTime::timeNow()) : startTime; }

TickRep StopWatch::stop() { return stopTime == 0 ? (stopTime = HighResolutionTime::timeNow()) : stopTime; }

TickRep StopWatch::lap() { return (lastLapTime = HighResolutionTime::timeNow()); }

TickRep StopWatch::lapTick() const { return lastLapTime > startTime ? (lastLapTime - startTime) : 0; }

TickRep StopWatch::thisLapTick() const
{
    return lastLapTime > startTime ? (HighResolutionTime::timeNow() - lastLapTime) : (HighResolutionTime::timeNow() - startTime);
}

TickRep StopWatch::durationTick() const { return stopTime > startTime ? (stopTime - startTime) : (HighResolutionTime::timeNow() - startTime); }

TimeConvType StopWatch::lapTime() const { return HighResolutionTime::asSeconds(lapTick()); }

TimeConvType StopWatch::thisLap() const { return HighResolutionTime::asSeconds(thisLapTick()); }

TimeConvType StopWatch::duration() const { return HighResolutionTime::asSeconds(durationTick()); }