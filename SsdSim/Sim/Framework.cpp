#include "Framework.h"

#include "FirmwareCore.h"
#include "Ipc/Message.h"
#include "JSONParser.h"

constexpr U32 SSDSIM_IPC_SIZE = 10 * 1024 * 1024;

Framework::Framework() :
	_State(State::Start),
	_NopCount(0)
{

}

void Framework::Init(const std::string& configFileName)
{
	int retValue;
	JSONParser parser;
	try
	{
		parser.Parse(configFileName);
	}
	catch (...)
	{
		throw (std::string("Failed to parse ") + configFileName);
	}

	SetupNandHal(parser);
	
	_MessageServer = std::make_shared<MessageServer>(SSDSIM_IPC_NAME, SSDSIM_IPC_SIZE);
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
		throw ss.str();
		return 0;
	};

	int retValue;

	try
	{
		retValue = parser.GetValueIntForAttribute("NandHalPreInit", "channels");
	}
	catch (JSONParser::Exception e)
	{
		throw (std::string("Failed to parse \'channels\' value. Expecting an \'int\'"));
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
		throw (std::string("Failed to parse \'devices\' value. Expecting an \'int\'"));
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
		throw (std::string("Failed to parse \'blocks\' value. Expecting an \'int\'"));
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
		throw (std::string("Failed to parse \'pages\' value. Expecting an \'int\'"));
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
		throw (std::string("Failed to parse \'bytes\' value. Expecting an \'int\'"));
	}
	constexpr U32 maxBytesValue = 16 * 1024;
	constexpr U32 minBytesValue = 4 * 1024;
	U32 bytes = validateValue(retValue, minBytesValue, maxBytesValue, "bytes");

	_NandHal.PreInit(channels, devices, blocks, pages, bytes);
}

void Framework::operator()()
{
	std::future<void> nandHal;
	std::future<void> firmwareMain;

	while (State::Exit != _State)
	{
		switch (_State)
		{
			case State::Start:
			{
				nandHal = std::async(std::launch::async, &NandHal::operator(), &_NandHal);
				firmwareMain = std::async(std::launch::async, &FirmwareCore::operator(), &_FirmwareCore);

				_State = State::Run;
			} break;

			case State::Run:
			{
				if (true == _MessageServer->HasMessage())
				{
					Message* message = _MessageServer->Pop();

					switch (message->_Type)
					{
                        case Message::Type::SIM_FRAMEWORK_COMMAND:
                        {
                            SimFrameworkCommand *command = (SimFrameworkCommand*)message->_Payload;
                            HandleSimFrameworkCommand(command);
                        } break;
					}

                    _MessageServer->DeallocateMessage(message);
				}
			} break;
		}
	}

	_FirmwareCore.Stop();
	_NandHal.Stop();
}

void Framework::HandleSimFrameworkCommand(SimFrameworkCommand *command)
{
    switch (command->_Code)
    {
        case SimFrameworkCommand::Code::Nop:
        {
            // Do nothing
            ++_NopCount;
        } break;

        case SimFrameworkCommand::Code::Exit:
        {
            _State = State::Exit;
        } break;
    }
}
