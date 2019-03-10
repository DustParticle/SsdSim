#include "pch.h"

#include <memory>

#include "HostCommShared.h"

using namespace HostCommTest;

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

	auto message = AllocateMessage<SimpleCommand>(client, false);
	ASSERT_NE(message, nullptr);
	message->_Data._Command = SimpleCommand::Command::SimpleCommand;
	client->Push(message);

	ASSERT_TRUE(server->HasMessage());

	auto receivedMessage = server->Pop();
	ASSERT_EQ(receivedMessage->_Data._Command, SimpleCommand::Command::SimpleCommand);
	ASSERT_FALSE(server->HasMessage());
	ASSERT_ANY_THROW(server->PushResponse(receivedMessage));             // Don't allow to response message without response flag
	ASSERT_NO_THROW(server->DeallocateMessage(receivedMessage));

	message = AllocateMessage<SimpleCommand>(client, true);
	ASSERT_NE(message, nullptr);
	message->_Data._Command = SimpleCommand::Command::SimpleCommand;
	client->Push(message);
	ASSERT_TRUE(server->HasMessage());
	receivedMessage = server->Pop();
	ASSERT_ANY_THROW(server->DeallocateMessage(receivedMessage));   // Don't allow to deallocate message with response flag
	ASSERT_NO_THROW(server->PushResponse(receivedMessage));			// Allow to response message
	ASSERT_TRUE(client->HasResponse());                             // Should have response
	auto responseMessage = client->PopResponse();
	client->DeallocateMessage(responseMessage);
}