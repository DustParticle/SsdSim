#pragma once
#ifndef __HostCommShared_h__
#define __HostCommShared_h__

#include "HostComm/Ipc/MessageServer.hpp"
#include "HostComm/Ipc/MessageClient.hpp"

namespace HostCommTest
{

class SimpleCommand
{
public:
	enum class Command
	{
		SimpleCommand
	};

public:
	Command _Command;
};

using SimpleCommandMessage = Message<SimpleCommand>;
using SimpleCommandMessageClient = MessageClient<SimpleCommand>;
using SimpleCommandMessageClientSharedPtr = std::shared_ptr<MessageClient<SimpleCommand>>;
using SimpleCommandMessageServer = MessageServer<SimpleCommand>;

template <class M>
Message<M>* AllocateMessage(std::shared_ptr<MessageClient<M>> client, bool expectResponse)
{
	auto message = client->AllocateMessage(sizeof(SimpleCommand), expectResponse);
	M* simpleCommand = (M*)message->_Payload;
	return message;
}

}

#endif