#ifndef __Framework_h__
#define __Framework_h__

#include <queue>

#include "Nand/NandHal.h"
#include "Ipc/MessageServer.h"

constexpr char SSDSIM_IPC_NAME[] = "SsdSim";

class Framework
{
public:
	Framework();

public:
	void operator()();

private:
	enum class State
	{
		Start,
		Run,
		Exit
	};

	State _State;

private:
    std::shared_ptr<MessageServer> _MessageServer;
	NandHal _NandHal;

public:
    U32 _NopCount;
};

#endif
