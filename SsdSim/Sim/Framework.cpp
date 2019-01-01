#include "Framework.h"

Framework::Framework() :
	_State(State::Start)
{
	//For now, hardcode NAND specifications
	//Later they will be loaded from configuration
	constexpr U8 channels = 4;
	constexpr U8 devices = 1;
	constexpr U32 blocks = 128;
	constexpr U32 pages = 256;
	constexpr U32 bytes = 8192;
	_NandHal.PreInit(channels, devices, blocks, pages, bytes);
}

void Framework::PushMessage(const Message& message)
{
	_Messages.push(message);
}

void Framework::Run()
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
			}break;
			case State::Run:
			{
				if (false == _Messages.empty())
				{
					auto message = _Messages.front();
					_Messages.pop();

					switch (message)
					{
						case Message::Exit:
						{
							_State = State::Exit;
						}break;
					}
				}
				
			}break;
		}
	}

	_NandHal.Stop();
}