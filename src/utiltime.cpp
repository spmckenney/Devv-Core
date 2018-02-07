/*
 * utiltime.cpp
 *
 *  Created on: Dec 27, 2017
 *      Author: Nick Williams
 */

#include "utiltime.h"

#include <atomic>
#include <chrono>
#include <time.h>
using namespace std::chrono;

static std::atomic<int64_t> nMockTime(0); //!< For unit testing

int64_t GetTime()
{
    int64_t mocktime = nMockTime.load(std::memory_order_relaxed);
    if (mocktime) return mocktime;

    time_t now = time(nullptr);
    return now;
}

void SetMockTime(int64_t nMockTimeIn)
{
    nMockTime.store(nMockTimeIn, std::memory_order_relaxed);
}

int64_t GetMockTime()
{
    return nMockTime.load(std::memory_order_relaxed);
}

int64_t GetTimeMillis()
{
	int64_t ms = system_clock::now().time_since_epoch().count();
    return ms;
}

int64_t GetTimeMicros()
{
	int64_t now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return now;
}

int64_t GetSystemTimeInSeconds()
{
    return GetTimeMicros()/1000000;
}

std::string DateTimeStrFormat(const char* format, int64_t nTime)
{
	time_t theTime = (time_t) nTime;
	char buff[100];
	strftime(buff, 100, format, localtime(&theTime));
    std::string timeStr(buff);
    return timeStr;
}


