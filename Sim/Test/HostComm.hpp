#pragma once
#ifndef __HostComm_h__
#define __HostComm_h__

#include "HostComm/Ipc/MessageServer.hpp"
#include "HostComm/Ipc/MessageClient.hpp"
#include "HostComm/CustomProtocol/CustomProtocolCommand.h"
#include "SimFramework/Framework.h"

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
	Command Command;
};

using SimpleCommandMessage = Message<SimpleCommand>;
using SimpleCommandMessageClient = MessageClient<SimpleCommand>;
using SimpleCommandMessageClientSharedPtr = std::shared_ptr<MessageClient<SimpleCommand>>;
using SimpleCommandMessageServer = MessageServer<SimpleCommand>;

using SimFrameworkMessage = Message<SimFrameworkCommand>;
using SimFrameworkMessageClient = MessageClient<SimFrameworkCommand>;
using SimFrameworkMessageClientSharedPtr = std::shared_ptr<MessageClient<SimFrameworkCommand>>;
using SimFrameworkMessageServer = MessageServer<SimFrameworkCommand>;

using CustomProtocolMessage = Message<CustomProtocolCommand>;
using CustomProtocolMessageClient = MessageClient<CustomProtocolCommand>;
using CustomProtocolMessageClientSharedPtr = std::shared_ptr<MessageClient<CustomProtocolCommand>>;
using CustomProtocolMessageServer = MessageServer<CustomProtocolCommand>;

template <class M>
Message<M>* AllocateMessage(std::shared_ptr<MessageClient<M>> client, U32 payloadSize, bool expectResponse)
{
	auto message = client->AllocateMessage(payloadSize, expectResponse);
	return message;
}

}

#endif