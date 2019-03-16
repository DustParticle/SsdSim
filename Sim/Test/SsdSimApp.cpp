#include "pch.h"

#define NOMINMAX
#include <windows.h>

#include <ctime>
#include <chrono>
#include <map>

#include "Test/gtest-cout.h"

#include "SimFramework/Framework.h"

#include "HostCommShared.h"

using namespace HostCommTest;

bool LaunchProcess(const char *pFile, const char *pArgs, SHELLEXECUTEINFO & ShExecInfo)
{
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = NULL;
	ShExecInfo.lpFile = pFile;
	ShExecInfo.lpParameters = pArgs;
	ShExecInfo.lpDirectory = NULL;
	ShExecInfo.nShow = SW_SHOW;
	ShExecInfo.hInstApp = NULL;

	return ShellExecuteEx(&ShExecInfo);
}

TEST(SsdSimApp, Basic)
{
	//Start the app
	GOUT("Starting SsdSim process.");
	SHELLEXECUTEINFO ShExecInfo = { 0 };
	ASSERT_TRUE(LaunchProcess("SsdSim.exe", "--hardwarespec Hardwareconfig/hardwarespec.json", ShExecInfo));

	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	DWORD exitCode = 0;
	ASSERT_TRUE(GetExitCodeProcess(ShExecInfo.hProcess, &exitCode));
	ASSERT_EQ(STILL_ACTIVE, exitCode);

	constexpr char* messagingName = "SsdSimMainMessageServer";	//TODO: define a way to get name

	//TODO: implement way to get server name
	auto client = std::make_shared<MessageClient<SimFrameworkCommand>>(messagingName);
	ASSERT_NE(nullptr, client);

	auto message = AllocateMessage<SimFrameworkCommand>(client, 0, false);
	ASSERT_NE(message, nullptr);
	message->_Data._Code = SimFrameworkCommand::Code::Exit;
	client->Push(message);

	if (ShExecInfo.hProcess)
	{
		GOUT("Waiting for process termination.");
		ASSERT_NE(WAIT_TIMEOUT, WaitForSingleObject(ShExecInfo.hProcess, 10000));
		GetExitCodeProcess(ShExecInfo.hProcess, &exitCode);
		CloseHandle(ShExecInfo.hProcess);
	}
}

TEST(SsdSimApp, Basic_Benchmark)
{
	//Start the app
	GOUT("Starting SsdSim process.");
	SHELLEXECUTEINFO ShExecInfo = { 0 };
	ASSERT_TRUE(LaunchProcess("SsdSim.exe", "--hardwarespec Hardwareconfig/hardwarespec.json", ShExecInfo));

	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	DWORD exitCode = 0;
	ASSERT_TRUE(GetExitCodeProcess(ShExecInfo.hProcess, &exitCode));
	ASSERT_EQ(STILL_ACTIVE, exitCode);

	constexpr char* messagingName = "SsdSimMainMessageServer";	//TODO: define a way to get name

	//TODO: implement way to get server name
	auto client = std::make_shared<MessageClient<SimFrameworkCommand>>(messagingName);
	ASSERT_NE(nullptr, client);

	using namespace std::chrono;

	constexpr U32 payloadSize = 4 * 1024;
	constexpr U32 commandCount = 10;
	Message<SimFrameworkCommand>* messages[commandCount];
	for (auto i = 0; i < commandCount; ++i)
	{
		messages[i] = AllocateMessage<SimFrameworkCommand>(client, payloadSize, true);
		ASSERT_NE(messages[i], nullptr);
	}

	duration<double> dataCmdMaxDuration = duration<double>::min();
	duration<double> dataCmdMinDuration = duration<double>::max();
	duration<double> dataCmdTotalTime = duration<double>::zero();
	std::map<U32, high_resolution_clock::time_point> t0s;
	for (auto i = 0; i < commandCount; ++i)
	{
		messages[i]->_Data._Code = SimFrameworkCommand::Code::DataOutLoopback;
		t0s.insert(std::make_pair(messages[i]->Id(), high_resolution_clock::now()));
		memset(messages[i]->_Payload, 0xaa, messages[i]->_PayloadSize);
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
		messages[i]->_Data._Code = SimFrameworkCommand::Code::DataInLoopback;
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
	message->_Data._Code = SimFrameworkCommand::Code::Exit;
	client->Push(message);

	if (ShExecInfo.hProcess)
	{
		GOUT("Waiting for process termination.");
		ASSERT_NE(WAIT_TIMEOUT, WaitForSingleObject(ShExecInfo.hProcess, 10000));
		CloseHandle(ShExecInfo.hProcess);
	}
}