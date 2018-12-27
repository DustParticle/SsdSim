#include "Framework.h"

Framework::Framework() :
	_State(State::Start)
{

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