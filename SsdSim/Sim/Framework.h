#ifndef __Framework_h__
#define __Framework_h__

#include <queue>

#include "Nand/NandHal.h"
#include "FirmwareCore.h"
#include "Ipc/MessageServer.h"

constexpr char SSDSIM_IPC_NAME[] = "SsdSim";

class SimFrameworkCommand
{
public:
    enum class Code
    {
        Nop,
        Exit
    };

public:
    Code _Code;
};

class Framework
{
public:
	Framework();

public:
	void operator()();

private:
    void handleSimFrameworkCommand(SimFrameworkCommand *command);

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
	FirmwareCore _FirmwareCore;

public:
    U32 _NopCount;
};

#endif
