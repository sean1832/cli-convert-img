#pragma once
#include <chrono>

enum class TimeUnit
{
	Second,
	Millisecond,
	Microsecond
};

class Timer
{
public:
	Timer(const char* name, TimeUnit unit = TimeUnit::Millisecond);

	void Stop();

	~Timer();
private:
	const char* name_;
	const TimeUnit unit_;
	bool stopped_;
	std::chrono::time_point<std::chrono::steady_clock> start_time_point_;
};

