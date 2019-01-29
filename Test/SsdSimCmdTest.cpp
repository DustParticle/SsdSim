#include "pch.h"

#include <future>
#include <array>
#include <chrono>
#include <assert.h>
#include <windows.h>

#include "gtest-cout.h"
#include "Nand/NandDevice.h"
#include "Nand/NandHal.h"
#include "Framework.h"
#include "Ipc/MessageClient.h"



Message* _allocateSimFrameworkCommand(std::shared_ptr<MessageClient> client, const SimFrameworkCommand::Code &code,
	const U32 &bufferSize = 0, const bool &expectsResponse = false)
{
	Message *message = client->AllocateMessage(Message::Type::SIM_FRAMEWORK_COMMAND, sizeof(SimFrameworkCommand) + bufferSize, expectsResponse);
	SimFrameworkCommand *command = (SimFrameworkCommand*)message->_Payload;
	command->_Code = code;
	return message;
}

bool shellExecute(const char *pFile, const char *pArgs, SHELLEXECUTEINFO & ShExecInfo)
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

TEST(A_SsdSimCmdExecute, Basic)
{
	//Start the framework
	SHELLEXECUTEINFO ShExecInfo = { 0 };
	ASSERT_TRUE(shellExecute("SsdSim.exe", "--nandspec nandspec.json", ShExecInfo));

	DWORD exitCode = 0;
	ASSERT_TRUE(GetExitCodeProcess(ShExecInfo.hProcess, &exitCode));
	ASSERT_EQ(STILL_ACTIVE, exitCode);

	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	std::shared_ptr<MessageClient> client = std::make_shared<MessageClient>(SSDSIM_IPC_NAME);
	ASSERT_NE(nullptr, client);

	Message *message = _allocateSimFrameworkCommand(client, SimFrameworkCommand::Code::Exit);
	client->Push(message);

	if (ShExecInfo.hProcess)
	{
		WaitForSingleObject(ShExecInfo.hProcess, INFINITE);
		CloseHandle(ShExecInfo.hProcess);
	}
}