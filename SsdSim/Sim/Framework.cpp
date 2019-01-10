#include "Framework.h"
#include "Ipc/Message.h"

constexpr U32 SSDSIM_IPC_SIZE = 10 * 1024 * 1024;

Framework::Framework() :
    _State(State::Start),
    _NopCount(0)
{
	//For now, hardcode NAND specifications
	//Later they will be loaded from configuration
	constexpr U8 channels = 4;
	constexpr U8 devices = 1;
	constexpr U32 blocks = 128;
	constexpr U32 pages = 256;
	constexpr U32 bytes = 8192;
	_NandHal.PreInit(channels, devices, blocks, pages, bytes);

    _MessageServer = std::make_shared<MessageServer>(SSDSIM_IPC_NAME, SSDSIM_IPC_SIZE);
}

void Framework::operator()()
{
	std::future<void> nandHal;

	while (State::Exit != _State)
	{
		switch (_State)
		{
			case State::Start:
			{
				nandHal = std::async(std::launch::async, &NandHal::operator(), &_NandHal);

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
                            handleSimFrameworkCommand(command);
                        } break;
					}

                    _MessageServer->DeallocateMessage(message);
				}
			} break;
		}
	}

	_NandHal.Stop();
}

void Framework::handleSimFrameworkCommand(SimFrameworkCommand *command)
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
