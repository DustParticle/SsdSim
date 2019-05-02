#include "pch.h"

#include <ctime>
#include <map>

#include "Test/gtest-cout.h"

#include "SimFramework/Framework.h"

#include "HostComm.hpp"

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

	auto message = AllocateMessage<SimFrameworkCommand>(client, 0, false);
	ASSERT_NE(message, nullptr);
	message->Data.Code = SimFrameworkCommand::Code::Exit;
	client->Push(message);

	//Give the Framework a chance to stop completely before next test
	// This is a work around until multiple servers can be created without collision
	std::this_thread::sleep_for(std::chrono::milliseconds(3000));
}

TEST(SimFramework, Basic_Benchmark)
{
	constexpr char* messagingName = "SsdSimMainMessageServer";	//TODO: define a way to get name

	Framework framework;
	ASSERT_NO_THROW(framework.Init("Hardwareconfig/hardwarespec.json"));

	auto fwFuture = std::async(std::launch::async, &(Framework::operator()), &framework);

	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	auto client = std::make_shared<MessageClient<SimFrameworkCommand>>(messagingName);
	ASSERT_NE(nullptr, client);

	using namespace std::chrono;
	high_resolution_clock::time_point t0;
	duration<double> allocMaxDuration = duration<double>::min();
	duration<double> allocMinDuration = duration<double>::max();
	duration<double> allocTotalTime = duration<double>::zero();

	constexpr U32 payloadSize = 4 * 1024;
	constexpr U32 commandCount = 10;
	Message<SimFrameworkCommand>* messages[commandCount];
	for (auto i = 0; i < commandCount; ++i)
	{
		t0 = high_resolution_clock::now();
		messages[i] = AllocateMessage<SimFrameworkCommand>(client, payloadSize, true);
		auto deltaT = duration_cast<duration<double>>(high_resolution_clock::now() - t0);
		if (deltaT < allocMinDuration) { allocMinDuration = deltaT; }
		if (deltaT > allocMaxDuration) { allocMaxDuration = deltaT; }
		allocTotalTime += deltaT;

		ASSERT_NE(messages[i], nullptr);
	}

	GOUT("Message allocation benchmark");
	GOUT("   Operations per second: " << (double)commandCount / allocTotalTime.count());
	GOUT("   Min time per operation: " << allocMinDuration.count());
	GOUT("   Max time per operation: " << allocMaxDuration.count());

	duration<double> dataCmdMaxDuration = duration<double>::min();
	duration<double> dataCmdMinDuration = duration<double>::max();
	duration<double> dataCmdTotalTime = duration<double>::zero();
	std::map<U32, high_resolution_clock::time_point> t0s;
	for (auto i = 0; i < commandCount; ++i)
	{
		messages[i]->Data.Code = SimFrameworkCommand::Code::DataOutLoopback;
		t0s.insert(std::make_pair(messages[i]->Id(), high_resolution_clock::now()));
		memset(messages[i]->Payload, 0xaa, messages[i]->PayloadSize);
		client->Push(messages[i]);
	}

	U32 responseReceivedCount = 0;
	while (responseReceivedCount < commandCount)
	{
		if (client->HasResponse())
		{
			messages[responseReceivedCount] = client->PopResponse();
			
			auto deltaT = duration_cast<duration<double>>(high_resolution_clock::now() - t0s.find(messages[responseReceivedCount]->Id())->second);
			if (deltaT < dataCmdMinDuration) { dataCmdMinDuration = deltaT; }
			if (deltaT > dataCmdMaxDuration) { dataCmdMaxDuration = deltaT; }
			dataCmdTotalTime += deltaT;
			
			responseReceivedCount++;
		}
	}

	GOUT("Data out benchmark");
	GOUT("   Operations per second: " << (double)commandCount / dataCmdTotalTime.count());
	GOUT("   Min time per operation: " << dataCmdMinDuration.count());
	GOUT("   Max time per operation: " << dataCmdMaxDuration.count());

	//--Data in benchmark
	dataCmdMaxDuration = duration<double>::min();
	dataCmdMinDuration = duration<double>::max();
	dataCmdTotalTime = duration<double>::zero();
	t0s.clear();
	for (auto i = 0; i < commandCount; ++i)
	{
		messages[i]->Data.Code = SimFrameworkCommand::Code::DataInLoopback;
		t0s.insert(std::make_pair(messages[i]->Id(), high_resolution_clock::now()));
		client->Push(messages[i]);
	}

	responseReceivedCount = 0;
	while (responseReceivedCount < commandCount)
	{
		if (client->HasResponse())
		{
			messages[responseReceivedCount] = client->PopResponse();

			auto deltaT = duration_cast<duration<double>>(high_resolution_clock::now() - t0s.find(messages[responseReceivedCount]->Id())->second);
			if (deltaT < dataCmdMinDuration) { dataCmdMinDuration = deltaT; }
			if (deltaT > dataCmdMaxDuration) { dataCmdMaxDuration = deltaT; }
			dataCmdTotalTime += deltaT;

			responseReceivedCount++;
		}
	}

	GOUT("Data in benchmark");
	GOUT("   Operations per second: " << (double)commandCount / dataCmdTotalTime.count());
	GOUT("   Min time per operation: " << dataCmdMinDuration.count());
	GOUT("   Max time per operation: " << dataCmdMaxDuration.count());

	//--Deallocate
	for (auto i = 0; i < commandCount; ++i)
	{
		client->DeallocateMessage(messages[i]);
	}

	auto message = AllocateMessage<SimFrameworkCommand>(client, 0, false);
	ASSERT_NE(message, nullptr);
	message->Data.Code = SimFrameworkCommand::Code::Exit;
	client->Push(message);

	//Give the Framework a chance to stop completely before next test
	// This is a work around until multiple servers can be created without collision
	std::this_thread::sleep_for(std::chrono::milliseconds(3000));
}