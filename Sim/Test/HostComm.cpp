#include "pch.h"

#include <memory>

#include "HostComm/Ipc/MessageServer.hpp"
#include "HostComm/Ipc/MessageClient.hpp"

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

SimpleCommandMessage* AllocateSimpleCommandMessage(SimpleCommandMessageClientSharedPtr client, const SimpleCommand::Command& command, bool expectResponse)
{
	auto message = client->AllocateMessage(sizeof(SimpleCommand), expectResponse);
	SimpleCommand* simpleCommand = (SimpleCommand*)message->_Payload;
	simpleCommand->_Command = SimpleCommand::Command::SimpleCommand;
	return message;
}

TEST(HostComm, Messaging_Basic)
{
	constexpr char* messagingName = "HostCommTest_Messaging";	//TODO: add random post-fix for parallel testing

	auto server = std::make_shared<SimpleCommandMessageServer>(messagingName, 8 * 1024);
	ASSERT_NE(server, nullptr);
	auto serverOpen = std::make_shared<SimpleCommandMessageServer>(messagingName);
	ASSERT_NE(serverOpen, nullptr);
	ASSERT_FALSE(server->HasMessage());

	auto client = std::make_shared<SimpleCommandMessageClient>(messagingName);
	ASSERT_NE(client, nullptr);

	auto message = AllocateSimpleCommandMessage(client, SimpleCommand::Command::SimpleCommand, false);
	ASSERT_NE(message, nullptr);
	client->Push(message);

	ASSERT_TRUE(server->HasMessage());

	auto receivedMessage = server->Pop();
	ASSERT_EQ(receivedMessage->_Data._Command, SimpleCommand::Command::SimpleCommand);
	ASSERT_FALSE(server->HasMessage());
	ASSERT_ANY_THROW(server->PushResponse(receivedMessage));             // Don't allow to response message without response flag
	ASSERT_NO_THROW(server->DeallocateMessage(receivedMessage));

	message = AllocateSimpleCommandMessage(client, SimpleCommand::Command::SimpleCommand, true);
	ASSERT_NE(message, nullptr);
	client->Push(message);
	ASSERT_TRUE(server->HasMessage());
	receivedMessage = server->Pop();
	ASSERT_ANY_THROW(server->DeallocateMessage(receivedMessage));   // Don't allow to deallocate message with response flag
	ASSERT_NO_THROW(server->PushResponse(receivedMessage));			// Allow to response message
	ASSERT_TRUE(client->HasResponse());                             // Should have response
	auto responseMessage = client->PopResponse();
	client->DeallocateMessage(responseMessage);
}