#include "Timer.h"

#include <iostream>

Timer::Timer(const char* name, const TimeUnit unit): name_(name), unit_(unit), stopped_(false)
{
	start_time_point_ = std::chrono::steady_clock::now();
}

void Timer::Stop()
{
	const auto end_time_point = std::chrono::steady_clock::now();

	long long start = std::chrono::time_point_cast<std::chrono::microseconds>(start_time_point_).time_since_epoch().count();
	long long end = std::chrono::time_point_cast<std::chrono::microseconds>(end_time_point).time_since_epoch().count();

	char unit[3];

	// convert 
	switch (unit_)
	{
		case TimeUnit::Second:
			start /= 1000000;
			end /= 1000000;
			unit[0] = 's';
			unit[1] = '\0';
			break;
		case TimeUnit::Millisecond:
			start /= 1000;
			end /= 1000;
			unit[0] = 'm';
			unit[1] = 's';
			break;
		case TimeUnit::Microsecond:
			unit[0] = 'u';
			unit[1] = 's';
			break;
		default:
			break;
	}


	std::cout << "Time elapsed: [" << name_ << "] " << (end - start) << unit << std::endl;
	stopped_ = true;
}

Timer::~Timer()
{
	if (!stopped_)
	{
		Stop();
	}
}
