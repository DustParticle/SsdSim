#ifndef __Framework_h__
#define __Framework_h__

#include <queue>
#include <exception>
#include <string>

#include "Nand/NandHal.h"
#include "FirmwareCore.h"
#include "Ipc/MessageServer.h"

class JSONParser;

class SimFrameworkCommand
{
public:
    enum class Code
    {
        Exit
    };

public:
    Code _Code;
};

class Framework
{
public:
	class Exception : public std::exception
	{
	public:
		Exception(std::string errMesg) : _ErrMesg(errMesg) {}
		const char* what() const noexcept { return _ErrMesg.c_str(); }
	private:
		std::string _ErrMesg;
	};

public:
	Framework();
	void Init(const std::string& nandConfigFilename);

public:
	void operator()();

private:
    void SetupNandHal(JSONParser& parser);
    void GetFirmwareCoreInfo(JSONParser& parser);
    void HandleSimFrameworkCommand(SimFrameworkCommand *command);

private:
	enum class State
	{
		Start,
		Run,
		Exit
	};

	State _State;

private:
    std::shared_ptr<MessageServer> _SimServer;
    std::shared_ptr<MessageServer> _ProtocolServer;
	NandHal _NandHal;
	FirmwareCore _FirmwareCore;
	std::string _RomCodePath;
};

#endif
