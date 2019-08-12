#include "FrameworkThread.h"

#include "BasicTypes.h"

FrameworkThread::FrameworkThread() :
	_StopFuture(_StopSignal.get_future())
{

}

void FrameworkThread::operator()()
{
    U32 counter = 0;
    bool quit = false;
	while (false == quit)
	{
        ++counter;
        if (counter == 1000)
        {
            quit = IsStopRequested();
            counter = 0;
        }
		Run();
	}
}

void FrameworkThread::Stop()
{
	_StopSignal.set_value();
}

bool FrameworkThread::IsStopRequested()
{
	if (_StopFuture.wait_for(std::chrono::nanoseconds(0)) == std::future_status::timeout)
	{
		return false;
	}
		
	return true;
}