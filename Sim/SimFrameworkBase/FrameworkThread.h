#ifndef __FrameworkThread_h__
#define __FrameworkThread_h__

#include <future>

class FrameworkThread
{
public:
	FrameworkThread();
	FrameworkThread(FrameworkThread && rhs) : _StopSignal(std::move(rhs._StopSignal)), _StopFuture(std::move(rhs._StopFuture))
	{
		
	}

	FrameworkThread & operator=(FrameworkThread && rhs)
	{
		_StopSignal = std::move(rhs._StopSignal);
		_StopFuture = std::move(rhs._StopFuture);
		return *this;
	}

public:
	void operator()();
	void Stop();

protected:
	virtual void Run() = 0;
	
	bool IsStopRequested();

private:
	std::promise<void> _StopSignal;
	std::future<void> _StopFuture;
};

#endif
