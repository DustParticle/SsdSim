#include "FrameworkThread.h"

FrameworkThread::FrameworkThread() :
	_StopFuture(_StopSignal.get_future())
{

}

void FrameworkThread::operator()()
{
	Run();
}

void FrameworkThread::Stop()
{
	_StopSignal.set_value();
}

bool FrameworkThread::IsStopRequested()
{
	if (_StopFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::timeout)
	{
		return false;
	}
		
	return true;
}