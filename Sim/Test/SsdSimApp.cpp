#include "pch.h"

#include <windows.h>

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
	std::cout << "Starting shell execute for SsdSim.exe." << std::endl;
	SHELLEXECUTEINFO ShExecInfo = { 0 };
	ASSERT_TRUE(LaunchProcess("SsdSim.exe", "--hardwarespec Hardwareconfig/hardwarespec.json", ShExecInfo));

	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	std::cout << "Verifying process active." << std::endl;
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
		std::cout << "Waiting for process termination." << std::endl;
		ASSERT_NE(WAIT_TIMEOUT, WaitForSingleObject(ShExecInfo.hProcess, 10000));
		CloseHandle(ShExecInfo.hProcess);
	}
}
