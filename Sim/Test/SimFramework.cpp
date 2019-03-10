#include "pch.h"

#include "SimFramework/Framework.h"

#include "HostCommShared.h"

using namespace HostCommTest;

TEST(SimFramework, LoadConfigFile)
{
	Framework framework;
	ASSERT_NO_THROW(framework.Init("Hardwareconfig/hardwarespec.json"));

	Framework framework2;
	ASSERT_ANY_THROW(framework2.Init("Hardwareconfig/hardwarebadvalue.json"));
}

TEST(SimFramework, Basic)
{
	constexpr char* messagingName = "SsdSimMainMessageServer";	//TODO: define a way to get name

	Framework framework;
	ASSERT_NO_THROW(framework.Init("Hardwareconfig/hardwarespec.json"));

	auto fwFuture = std::async(std::launch::async, &(Framework::operator()), &framework);

	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	auto client = std::make_shared<MessageClient<SimFrameworkCommand>>(messagingName);
	ASSERT_NE(nullptr, client);

	auto message = AllocateMessage<SimFrameworkCommand>(client, false);
	ASSERT_NE(message, nullptr);
	message->_Data._Code = SimFrameworkCommand::Code::Exit;
	client->Push(message);
}