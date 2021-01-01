#include "Time.h"

#include <chrono>
#include <ratio>

using TimeResolution = std::chrono::microseconds;

TickRep Time::timeNow()
{
    using namespace std::chrono;
    return duration_cast<TimeResolution>(steady_clock::now().time_since_epoch()).count();
}

TickRep Time::clockTimeNow()
{
    using namespace std::chrono;
    return duration_cast<TimeResolution>(system_clock::now().time_since_epoch()).count();
}

TimeConvType Time::asSeconds(const TickRep& tickValue)
{
    using namespace std::chrono;
    return duration_cast<duration<TimeConvType>>(TimeResolution(tickValue)).count();
}

TimeConvType Time::asMinutes(const TickRep& tickValue)
{
    using namespace std::chrono;
    return duration_cast<duration<TimeConvType,std::ratio<60>>>(TimeResolution(tickValue)).count();
}

TimeConvType Time::asHours(const TickRep& tickValue)
{
    using namespace std::chrono;
    return duration_cast<duration<TimeConvType, std::ratio<3600>>>(TimeResolution(tickValue)).count();
}

TimeConvType Time::asDays(const TickRep& tickValue)
{
    using namespace std::chrono;
    return duration_cast<duration<TimeConvType, std::ratio<86400>>>(TimeResolution(tickValue)).count();
}

TickRep Time::addSeconds(const TickRep& tickValue, TimeConvType seconds)
{
    using namespace std::chrono;
    duration<TimeConvType> tickDur(asSeconds(tickValue));
    duration<TimeConvType> secondsToAdd(seconds);
    tickDur += secondsToAdd;
    return duration_cast<TimeResolution>(tickDur).count();
}

TickRep Time::addMinutes(const TickRep& tickValue, TimeConvType minutes)
{
    using namespace std::chrono;
    duration<TimeConvType, std::ratio<60>> tickDur(asMinutes(tickValue));
    duration<TimeConvType, std::ratio<60>> minutesToAdd(minutes);
    tickDur += minutesToAdd;
    return duration_cast<TimeResolution>(tickDur).count();
}

TickRep Time::addHours(const TickRep& tickValue, TimeConvType hours)
{
    using namespace std::chrono;
    duration<TimeConvType, std::ratio<3600>> tickDur(asHours(tickValue));
    duration<TimeConvType, std::ratio<3600>> hoursToAdd(hours);
    tickDur += hoursToAdd;
    return duration_cast<TimeResolution>(tickDur).count();

}

TickRep Time::addDays(const TickRep& tickValue, TimeConvType days)
{
    using namespace std::chrono;
    duration<TimeConvType, std::ratio<86400>> tickDur(asDays(tickValue));
    duration<TimeConvType, std::ratio<86400>> daysToAdd(days);
    tickDur += daysToAdd;
    return duration_cast<TimeResolution>(tickDur).count();
}
