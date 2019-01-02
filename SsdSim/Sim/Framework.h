#ifndef __Framework_h__
#define __Framework_h__

#include <queue>

#include "Nand/NandHal.h"

class Framework
{
public:
	Framework();

public:
	enum class Message
	{
		Exit
	};

	void PushMessage(const Message& message);

public:
	void operator()();

private:
	void Run();

private:
	enum class State
	{
		Start,
		Run,
		Exit
	};

	State _State;

private:
	std::queue<Message> _Messages;

private:
	NandHal _NandHal;
};

#endif
