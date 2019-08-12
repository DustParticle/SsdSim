#include "Framework.h"

#include "SimFrameworkBase/JSONParser.h"
#include "FirmwareCore.h"
#include "HostComm/Ipc/Message.hpp"
#include "HostComm/CustomProtocol/CustomProtocolCommand.h"

constexpr U32 MaxIpcServer = 10;

Framework::Framework() :
	_State(State::Start)
{
    _NandHal = std::make_shared<NandHal>();
    _BufferHal = std::make_shared<BufferHal>();
    _FirmwareCore = std::make_shared<FirmwareCore>();
}

void Framework::Init(const std::string& configFileName, std::string ipcNamesPrefix)
{
	JSONParser parser;
	try
	{
		parser.Parse(configFileName);
	}
	catch (...)
	{
		throw Exception("Failed to parse " + configFileName);
	}

    // Keep this order
    SetupBufferHal(parser);
	SetupNandHal(parser);
	GetFirmwareCoreInfo(parser);

    std::string simServerIpcName;
    std::string customProtocolIpcName;
    if (ipcNamesPrefix.empty())
    {
        simServerIpcName = "SsdSimMainMessageServer";
        customProtocolIpcName = "SsdSimCustomProtocolServer";
        _SimServer = std::make_shared<MessageServer<SimFrameworkCommand>>(simServerIpcName.c_str(), 8 * 1024 * 1024);
        _ProtocolServer = std::make_shared<MessageServer<CustomProtocolCommand>>(customProtocolIpcName.c_str(), 8 * 1024 * 1024);
    }
    else
    {
        U32 serverId = 0;
        while(serverId < MaxIpcServer)
        {
            try
            {
                simServerIpcName = ipcNamesPrefix + "MainMessageServer" + std::to_string(serverId);
                customProtocolIpcName = ipcNamesPrefix + "CustomProtocolServer" + std::to_string(serverId);
                _SimServer = std::make_shared<MessageServer<SimFrameworkCommand>>(simServerIpcName.c_str(), 8 * 1024 * 1024, false);
                _ProtocolServer = std::make_shared<MessageServer<CustomProtocolCommand>>(customProtocolIpcName.c_str(), 8 * 1024 * 1024, false);
            }
            catch (...)
            {
                // try next id
                serverId++;
                continue;
            }

            // found the available ipc servers
            break;
        }

        if (serverId == MaxIpcServer)
        {
            throw Exception("Failed to create ipc servers");
        }
    }

    _FirmwareCore->SetIpcNames(customProtocolIpcName);
}

void Framework::SetupNandHal(JSONParser& parser)
{
	auto validateValue = [](int value, int min, int max, const std::string& name)
	{
		if ((min <= value) && (value <= max))
		{
			return value;
		}

		std::ostringstream ss;
		ss << name << " value of " << value << " is out of range. Expected to be between [" << min << ", " << max << "]";
		throw Exception(ss.str());
	};

	int retValue;

	try
	{
		retValue = parser.GetValueIntForAttribute("NandHalPreInit", "channels");
	}
	catch (JSONParser::Exception e)
	{
		throw Exception("Failed to parse \'channels\' value. Expecting an \'int\'");
	}
	constexpr U8 maxChannelsValue = 8;
	constexpr U8 minChannelsValue = 1;
	U8 channels = validateValue(retValue, minChannelsValue, maxChannelsValue, "channels");

	try
	{
		retValue = parser.GetValueIntForAttribute("NandHalPreInit", "devices");
	}
	catch (JSONParser::Exception e)
	{
		throw Exception("Failed to parse \'devices\' value. Expecting an \'int\'");
	}
	constexpr U8 maxDevicesValue = 8;
	constexpr U8 minDevicesValue = 1;
	U8 devices = validateValue(retValue, minDevicesValue, maxDevicesValue, "devices");

	try
	{
		retValue = parser.GetValueIntForAttribute("NandHalPreInit", "blocks");
	}
	catch (JSONParser::Exception e)
	{
		throw Exception("Failed to parse \'blocks\' value. Expecting an \'int\'");
	}
	constexpr U32 maxBlocksValue = 32 * 1024;
	constexpr U32 minBlocksValue = 128;
	U32 blocks = validateValue(retValue, minBlocksValue, maxBlocksValue, "blocks");

	try
	{
		retValue = parser.GetValueIntForAttribute("NandHalPreInit", "pages");
	}
	catch (JSONParser::Exception e)
	{
		throw Exception("Failed to parse \'pages\' value. Expecting an \'int\'");
	}
	constexpr U32 maxPagesValue = 512;
	constexpr U32 minPagesValue = 32;
	U32 pages = validateValue(retValue, minPagesValue, maxPagesValue, "pages");

	try
	{
		retValue = parser.GetValueIntForAttribute("NandHalPreInit", "bytes");
	}
	catch (JSONParser::Exception e)
	{
		throw Exception("Failed to parse \'bytes\' value. Expecting an \'int\'");
	}
	constexpr U32 maxBytesValue = 16 * 1024;
	constexpr U32 minBytesValue = 4 * 1024;
	U32 bytes = validateValue(retValue, minBytesValue, maxBytesValue, "bytes");

	_NandHal->PreInit(channels, devices, blocks, pages, bytes);
	_NandHal->Init(_BufferHal.get());
}

void Framework::SetupBufferHal(JSONParser& parser)
{
    U32 maxBufferSizeInKB;
    try
    {
        maxBufferSizeInKB = parser.GetValueIntForAttribute("BufferHalPreInit", "kbs");
    }
    catch (JSONParser::Exception e)
    {
        throw Exception("Failed to parse \'kbs\' value. Expecting an \'int\'");
    }

    _BufferHal->PreInit(maxBufferSizeInKB);
}

void Framework::GetFirmwareCoreInfo(JSONParser& parser)
{
	try
	{
		this->_RomCodePath = parser.GetValueStringForAttribute("RomCode", "path");
	}
	catch (JSONParser::Exception e)
	{
		throw Exception("Failed to parse \'rom code path\' value. Expecting an \'string\'");
	}
}

void Framework::operator()()
{
	std::future<void> nandHal;
	std::future<void> firmwareMain;

    // Load ROM
	_FirmwareCore->SetHalComponents(_NandHal, _BufferHal);
	_FirmwareCore->SetExecute(this->_RomCodePath);

    while (State::Exit != _State)
	{
		switch (_State)
		{
			case State::Start:
			{
				nandHal = std::async(std::launch::async, &NandHal::operator(), _NandHal);
				firmwareMain = std::async(std::launch::async, &FirmwareCore::operator(), _FirmwareCore);

				_State = State::Run;
			} break;

			case State::Run:
			{
				if (true == _SimServer->HasMessage())
				{
                    Message<SimFrameworkCommand>* message = _SimServer->Pop();

					switch (message->Data.Code)
					{
                        case SimFrameworkCommand::Code::Exit:
                        {
                            _State = State::Exit;
							_SimServer->DeallocateMessage(message);
                        } break;
						case SimFrameworkCommand::Code::DataOutLoopback:
						{
							//Get data from host
							auto buffer = std::make_unique<U8[]>(message->PayloadSize);
							memcpy_s(buffer.get(), message->PayloadSize, message->Payload, message->PayloadSize);
							_SimServer->PushResponse(message->Id());
						} break;
						case SimFrameworkCommand::Code::DataInLoopback:
						{
							//Send data to host
							auto buffer = std::make_unique<U8[]>(message->PayloadSize);
							memcpy_s(message->Payload, message->PayloadSize, buffer.get(), message->PayloadSize);
							_SimServer->PushResponse(message->Id());
						} break;
					}

                    
				}
			} break;
		}
	}

	_FirmwareCore->Stop();
	_NandHal->Stop();

    firmwareMain.wait();
    nandHal.wait();

    _FirmwareCore->Unload();
}
