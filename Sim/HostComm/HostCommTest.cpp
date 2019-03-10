#include "Test/pch.h"

#include "HostComm/Ipc/MessageServer.hpp"

TEST(HostComm, MessageSystem_NopWithResponse)
{
	constexpr char name[] = "Test";
	constexpr U32 size = 10 * 1024 * 1024;

	std::shared_ptr<MessageServer<CustomProtocolCommand>> server = std::make_shared<MessageServer<CustomProtocolCommand>>(name, size);
	ASSERT_NE(nullptr, server);

	std::shared_ptr<MessageClient<CustomProtocolCommand>> client = std::make_shared<MessageClient<CustomProtocolCommand>>(name);
	ASSERT_NE(nullptr, client);

	/* Send message without response */
	Message<CustomProtocolCommand> *message = allocateCustomProtocolCommand(client, CustomProtocolCommand::Code::Nop);
	ASSERT_NE(nullptr, message);
	client->Push(message);
	ASSERT_TRUE(server->HasMessage());
	Message<CustomProtocolCommand> *popMessage = server->Pop();
	ASSERT_ANY_THROW(server->PushResponse(popMessage));             // Don't allow to response message without response flag
	ASSERT_NO_THROW(server->DeallocateMessage(popMessage));         // Allow to deallocate message

	/* Send message with response */
	Message<CustomProtocolCommand> *messageWithResponse = allocateCustomProtocolCommand(client, CustomProtocolCommand::Code::Nop, 0, true);
	ASSERT_NE(nullptr, messageWithResponse);
	client->Push(messageWithResponse);
	ASSERT_TRUE(server->HasMessage());
	Message<CustomProtocolCommand> *popMessageWithResponse = server->Pop();
	ASSERT_ANY_THROW(server->DeallocateMessage(popMessageWithResponse));    // Don't allow to deallocate message with response flag
	ASSERT_NO_THROW(server->PushResponse(popMessageWithResponse));          // Allow to response message
	ASSERT_TRUE(client->HasResponse());                                     // Should have response
	Message<CustomProtocolCommand> *responseMessage = client->PopResponse();
	client->DeallocateMessage(responseMessage);
}