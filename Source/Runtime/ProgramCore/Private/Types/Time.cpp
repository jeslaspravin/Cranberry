/*!
 * \file Time.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Types/Time.h"
#include "String/String.h"
#include "Types/CoreDefines.h"

#include <chrono>
#include <ratio>
#include <time.h>

// Will be implemented in platform specific TU
template <typename Resolution>
TickRep fromPlatformTime(int64 platformTick);

using TimeResolution = std::chrono::microseconds;
using TimeHighResolution = std::chrono::nanoseconds;


template <bool bIsHighRes = false>
class TimeHelper
{
public:
    using Resolution = std::conditional_t<bIsHighRes, TimeHighResolution, TimeResolution>;
public:
    FORCE_INLINE static TickRep timeNow()
    {
        using namespace std::chrono;
        return duration_cast<Resolution>(steady_clock::now().time_since_epoch()).count();
    }

    FORCE_INLINE static TickRep clockTimeNow()
    {
        using namespace std::chrono;
        return duration_cast<Resolution>(system_clock::now().time_since_epoch()).count();
    }

    FORCE_INLINE static TimeConvType asSeconds(const TickRep& tickValue)
    {
        using namespace std::chrono;
        return duration_cast<duration<TimeConvType>>(Resolution(tickValue)).count();
    }

    FORCE_INLINE static TimeConvType asMinutes(const TickRep& tickValue)
    {
        using namespace std::chrono;
        return duration_cast<duration<TimeConvType, std::ratio<60>>>(Resolution(tickValue)).count();
    }

    FORCE_INLINE static TimeConvType asHours(const TickRep& tickValue)
    {
        using namespace std::chrono;
        return duration_cast<duration<TimeConvType, std::ratio<3600>>>(Resolution(tickValue)).count();
    }

    FORCE_INLINE static TimeConvType asDays(const TickRep& tickValue)
    {
        using namespace std::chrono;
        return duration_cast<duration<TimeConvType, std::ratio<86400>>>(Resolution(tickValue)).count();
    }

    //FORCE_INLINE static String asString(const TickRep& tickValue, const String& formatStr)
    //{
    //    using namespace std::chrono;
    //    steady_clock::time_point tickTimePoint(Resolution(tickValue));
    //    std::tm timeBuffer;
    //
    //    std::stringstream outStr;
    //    outStr << put_time(localtime_s(&tickTimePoint, &timeBuffer), formatStr.getChar());
    //    return outStr.str();
    //}

    FORCE_INLINE static TickRep addSeconds(const TickRep& tickValue, TimeConvType seconds)
    {
        using namespace std::chrono;
        duration<TimeConvType> tickDur(asSeconds(tickValue));
        duration<TimeConvType> secondsToAdd(seconds);
        tickDur += secondsToAdd;
        return duration_cast<Resolution>(tickDur).count();
    }

    FORCE_INLINE static TickRep addMinutes(const TickRep& tickValue, TimeConvType minutes)
    {
        using namespace std::chrono;
        duration<TimeConvType, std::ratio<60>> tickDur(asMinutes(tickValue));
        duration<TimeConvType, std::ratio<60>> minutesToAdd(minutes);
        tickDur += minutesToAdd;
        return duration_cast<Resolution>(tickDur).count();
    }

    FORCE_INLINE static TickRep addHours(const TickRep& tickValue, TimeConvType hours)
    {
        using namespace std::chrono;
        duration<TimeConvType, std::ratio<3600>> tickDur(asHours(tickValue));
        duration<TimeConvType, std::ratio<3600>> hoursToAdd(hours);
        tickDur += hoursToAdd;
        return duration_cast<Resolution>(tickDur).count();
    }

    FORCE_INLINE static TickRep addDays(const TickRep& tickValue, TimeConvType days)
    {
        using namespace std::chrono;
        duration<TimeConvType, std::ratio<86400>> tickDur(asDays(tickValue));
        duration<TimeConvType, std::ratio<86400>> daysToAdd(days);
        tickDur += daysToAdd;
        return duration_cast<Resolution>(tickDur).count();
    }

    FORCE_INLINE static TickRep fromPlatformTime(int64 platformTick)
    {
        return ::fromPlatformTime<Resolution>(platformTick);
    }
};

// namespace Time

TickRep Time::timeNow()
{
    return TimeHelper<false>::timeNow();
}

TickRep Time::clockTimeNow()
{
    return TimeHelper<false>::clockTimeNow();
}

TimeConvType Time::asSeconds(const TickRep& tickValue)
{
    return TimeHelper<false>::asSeconds(tickValue);
}

TimeConvType Time::asMinutes(const TickRep& tickValue)
{
    return TimeHelper<false>::asMinutes(tickValue);
}

TimeConvType Time::asHours(const TickRep& tickValue)
{
    return TimeHelper<false>::asHours(tickValue);
}

TimeConvType Time::asDays(const TickRep& tickValue)
{
    return TimeHelper<false>::asDays(tickValue);
}

TickRep Time::addSeconds(const TickRep& tickValue, TimeConvType seconds)
{
    return TimeHelper<false>::addSeconds(tickValue, seconds);
}

TickRep Time::addMinutes(const TickRep& tickValue, TimeConvType minutes)
{
    return TimeHelper<false>::addMinutes(tickValue, minutes);
}

TickRep Time::addHours(const TickRep& tickValue, TimeConvType hours)
{
    return TimeHelper<false>::addHours(tickValue, hours);

}

TickRep Time::addDays(const TickRep& tickValue, TimeConvType days)
{
    return TimeHelper<false>::addDays(tickValue, days);
}

TickRep Time::fromPlatformTime(int64 platformTick)
{
    return TimeHelper<false>::fromPlatformTime(platformTick);
}

// namespace HighResolutionTime

TickRep HighResolutionTime::timeNow()
{
    return TimeHelper<true>::timeNow();
}

TickRep HighResolutionTime::clockTimeNow()
{
    return TimeHelper<true>::clockTimeNow();
}

TimeConvType HighResolutionTime::asSeconds(const TickRep& tickValue)
{
    return TimeHelper<true>::asSeconds(tickValue);
}

TimeConvType HighResolutionTime::asMinutes(const TickRep& tickValue)
{
    return TimeHelper<true>::asMinutes(tickValue);
}

TimeConvType HighResolutionTime::asHours(const TickRep& tickValue)
{
    return TimeHelper<true>::asHours(tickValue);
}

TimeConvType HighResolutionTime::asDays(const TickRep& tickValue)
{
    return TimeHelper<true>::asDays(tickValue);
}

TickRep HighResolutionTime::addSeconds(const TickRep& tickValue, TimeConvType seconds)
{
    return TimeHelper<true>::addSeconds(tickValue, seconds);
}

TickRep HighResolutionTime::addMinutes(const TickRep& tickValue, TimeConvType minutes)
{
    return TimeHelper<true>::addMinutes(tickValue, minutes);
}

TickRep HighResolutionTime::addHours(const TickRep& tickValue, TimeConvType hours)
{
    return TimeHelper<true>::addHours(tickValue, hours);

}

TickRep HighResolutionTime::addDays(const TickRep& tickValue, TimeConvType days)
{
    return TimeHelper<true>::addDays(tickValue, days);
}

TickRep HighResolutionTime::fromPlatformTime(int64 platformTick)
{
    return TimeHelper<true>::fromPlatformTime(platformTick);
}

// Stopwatch

StopWatch::StopWatch(bool bStart /*= true*/)
{
    if (bStart)
    {
        startTime = HighResolutionTime::timeNow();
    }
}

TickRep StopWatch::start()
{
    return (startTime = HighResolutionTime::timeNow());
}

TickRep StopWatch::stop()
{
    return (stopTime = HighResolutionTime::timeNow());
}

TickRep StopWatch::lap()
{
    return (lastLapTime = HighResolutionTime::timeNow());
}

TimeConvType StopWatch::lapTime() const
{
    return lastLapTime > startTime ? HighResolutionTime::asSeconds(lastLapTime - startTime) : 0;
}

TimeConvType StopWatch::thisLap() const
{
    return lastLapTime > startTime? HighResolutionTime::asSeconds(HighResolutionTime::timeNow() - lastLapTime) : HighResolutionTime::asSeconds(HighResolutionTime::timeNow() - startTime);
}

TimeConvType StopWatch::duration() const
{
    return stopTime > startTime ? HighResolutionTime::asSeconds(lastLapTime - startTime) : HighResolutionTime::asSeconds(HighResolutionTime::timeNow() - startTime);
}