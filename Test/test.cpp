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
#include "Ipc/MessageClient.hpp"
#include "ServerNames.h"
#include "Interfaces/CustomProtocolCommand.h"

Message<SimFrameworkCommand>* allocateSimFrameworkCommand(std::shared_ptr<MessageClient<SimFrameworkCommand>> client, const SimFrameworkCommand::Code &code)
{
    Message<SimFrameworkCommand> *message = client->AllocateMessage(sizeof(SimFrameworkCommand), false);
    SimFrameworkCommand *command = (SimFrameworkCommand*)message->_Payload;
    command->_Code = code;
    return message;
}

Message<CustomProtocolCommand>* allocateCustomProtocolCommand(std::shared_ptr<MessageClient<CustomProtocolCommand>> client, const CustomProtocolCommand::Code &code,
    const U32 &bufferSize = 0, const bool &expectsResponse = false)
{
    Message<CustomProtocolCommand> *message = client->AllocateMessage(sizeof(CustomProtocolCommand) + bufferSize, expectsResponse);
    CustomProtocolCommand *command = (CustomProtocolCommand*)message->_Payload;
    command->Command = code;
    return message;
}

TEST(LoadConfigFile, Basic)
{
	Framework framework;
    ASSERT_NO_THROW(framework.Init("Hardwareconfig/hardwarespec.json"));
}

TEST(LoadConfigFile, Negative)
{
	//Expect throw an exception
    Framework framework;
    ASSERT_ANY_THROW(framework.Init("Hardwareconfig/hardwarebadvalue.json"));
}

TEST(NandDeviceTest, Basic) {
	constexpr U32 blockCount = 64;
	constexpr U32 pagesPerBlock = 256;
	constexpr U32 bytesPerPage = 8192;

	NandDevice nandDevice(blockCount, pagesPerBlock, bytesPerPage);

	U8 pDataToWrite[bytesPerPage];
	for (auto i(0); i < sizeof(pDataToWrite); ++i)
	{
		pDataToWrite[i] = i % 255;
	}
	U8 pDataRead[bytesPerPage];
	U8 pErasedBuffer[bytesPerPage];
	std::memset(pErasedBuffer, NandBlock::ERASED_PATTERN, sizeof(pErasedBuffer));

	tBlockInDevice block;
	block._ = 0;
	for ( ; block._ < blockCount; ++block._)
	{
		tPageInBlock page;
		page._ = 0;
		for (; page._ < pagesPerBlock; ++page._)
		{
			nandDevice.WritePage(block, page, pDataToWrite);
			
			nandDevice.ReadPage(block, page, pDataRead);
			auto result = std::memcmp(pDataToWrite, pDataRead, bytesPerPage);
			ASSERT_EQ(0, result);
		}

		nandDevice.EraseBlock(block);
		for (; page._ < pagesPerBlock; ++page._)
		{
			nandDevice.ReadPage(block, page, pDataRead);
			auto result = std::memcmp(pErasedBuffer, pDataRead, bytesPerPage);
			ASSERT_EQ(0, result);
		}
	}
}

TEST(NandHalTest, Basic) {
	constexpr U8 channels = 4;
	constexpr U8 devices = 2;
	constexpr U32 blocks = 64;
	constexpr U32 pages = 256;
	constexpr U32 bytes = 8192;

	NandHal nandHal;
	nandHal.PreInit(channels, devices, blocks, pages, bytes);
	nandHal.Init();

	U8 pDataToWrite[bytes];
	for (auto i(0); i < sizeof(pDataToWrite); ++i)
	{
		pDataToWrite[i] = i % 255;
	}
	U8 pDataRead[bytes];
	U8 pErasedBuffer[bytes];
	std::memset(pErasedBuffer, NandBlock::ERASED_PATTERN, sizeof(pErasedBuffer));

	tChannel channel;
	channel._ = 0;
	for (; channel._ < channels; ++channel._)
	{
		tDeviceInChannel device;
		device._ = 0;
		for (; device._ < devices; ++device._)
		{
			tBlockInDevice block;
			block._ = 0;
			for (; block._ < blocks; ++block._)
			{
				tPageInBlock page;
				page._ = 0;
				for (; page._ < pages; ++page._)
				{
					nandHal.WritePage(channel, device, block, page, pDataToWrite);

					nandHal.ReadPage(channel, device, block, page, pDataRead);
					auto result = std::memcmp(pDataToWrite, pDataRead, bytes);
					ASSERT_EQ(0, result);
				}

				nandHal.EraseBlock(channel, device, block);
				for (; page._ < pages; ++page._)
				{
					nandHal.ReadPage(channel, device, block, page, pDataRead);
					auto result = std::memcmp(pErasedBuffer, pDataRead, bytes);
					ASSERT_EQ(0, result);
				}
			}
		}
	}
}

TEST(NandHalTest, BasicCommandQueue)
{
	constexpr U8 channels = 4;
	constexpr U8 devices = 2;
	constexpr U32 blocks = 64;
	constexpr U32 pages = 256;
	constexpr U32 bytes = 8192;
	constexpr U8 bufferCount = channels * 4;
	constexpr U32 commandCount = channels * devices * blocks * pages;

	std::array<U8[bytes], bufferCount> writeBuffers;
	for (auto buffer : writeBuffers)
	{
		for (auto i(0); i < bytes; ++i)
		{
			buffer[i] = i % 255;
		}
	}

	std::array<U8[bytes], bufferCount> readBuffers;

	U8 pErasedBuffer[bytes];
	std::memset(pErasedBuffer, NandBlock::ERASED_PATTERN, sizeof(pErasedBuffer));
	
	NandHal nandHal;
	nandHal.PreInit(channels, devices, blocks, pages, bytes);
	nandHal.Init();

	std::future<void> nandHalFuture;

	nandHalFuture = std::async(std::launch::async, &NandHal::operator(), &nandHal);

	NandHal::CommandDesc commandDesc;
	commandDesc.Channel._ = 0;
	commandDesc.Device._ = 0;
	commandDesc.Block._ = 0;
	commandDesc.Page._ = 0;
	for (auto c(0); c < commandCount;)
	{
		for (auto i(0); i < bufferCount; ++i, ++c)
		{
			commandDesc.Operation = NandHal::CommandDesc::Op::WRITE;
			commandDesc.Buffer = writeBuffers[i];
			nandHal.QueueCommand(commandDesc);

			commandDesc.Operation = NandHal::CommandDesc::Op::READ;
			commandDesc.Buffer = readBuffers[i];
			nandHal.QueueCommand(commandDesc);

			if (++commandDesc.Channel._ >= channels)
			{
				commandDesc.Channel._ = 0;
				if (++commandDesc.Device._ >= devices)
				{
					commandDesc.Device._ = 0;
					if (++commandDesc.Page._ >= pages)
					{
						commandDesc.Page._ = 0;
						assert(commandDesc.Block._ < blocks);	//let's not go pass this boundary
						++commandDesc.Block._;
					}
				}
			}
		}

		while (false == nandHal.IsCommandQueueEmpty());
	}

	nandHal.Stop();
}

TEST(SimFramework, Basic)
{
	Framework framework;
    ASSERT_NO_THROW(framework.Init("Hardwareconfig/hardwarespec.json"));

	auto fwFuture = std::async(std::launch::async, &(Framework::operator()), &framework);

	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    std::shared_ptr<MessageClient<SimFrameworkCommand>> client = std::make_shared<MessageClient<SimFrameworkCommand>>(SSDSIM_IPC_NAME);
    ASSERT_NE(nullptr, client);

    Message<SimFrameworkCommand> *message = allocateSimFrameworkCommand(client, SimFrameworkCommand::Code::Exit);
    client->Push(message);
}

TEST(SimFramework, Benchmark)
{
    constexpr U32 loopCount = 10;

	Framework framework;
    ASSERT_NO_THROW(framework.Init("Hardwareconfig/hardwarespec.json"));

    auto fwFuture = std::async(std::launch::async, &(Framework::operator()), &framework);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    std::shared_ptr<MessageClient<CustomProtocolCommand>> protocolClient = std::make_shared<MessageClient<CustomProtocolCommand>>(PROTOCOL_IPC_NAME);
    ASSERT_NE(nullptr, protocolClient);

    Message<CustomProtocolCommand> *message;
    CustomProtocolCommand *command;

    // Request to load RomCode
    message = allocateCustomProtocolCommand(protocolClient, CustomProtocolCommand::Code::DownloadAndExecute, 0, true);
    command = (CustomProtocolCommand*)message->_Payload;
    memcpy(command->Payload.DownloadAndExecute.CodeName, ".\\TestCode.dll", sizeof(".\\TestCode.dll"));
    protocolClient->Push(message);

    //// Wait for response
    while (!protocolClient->HasResponse())
    {
        // Do nothing
    }

    message = protocolClient->PopResponse();
    command = &message->_Data;
    ASSERT_EQ(CustomProtocolCommand::Code::DownloadAndExecute, command->Command);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Start benchmark
    message = allocateCustomProtocolCommand(protocolClient, CustomProtocolCommand::Code::BenchmarkStart);
    protocolClient->Push(message);

    for (U32 i = 0; i < loopCount; ++i)
    {
        // Push Nop
        message = allocateCustomProtocolCommand(protocolClient, CustomProtocolCommand::Code::Nop);
        protocolClient->Push(message);
    }

    // End benchmark
    // This message requires response
    message = allocateCustomProtocolCommand(protocolClient, CustomProtocolCommand::Code::BenchmarkEnd, 0, true);
    protocolClient->Push(message);

    //// Wait for response
    while (!protocolClient->HasResponse())
    {
        // Do nothing
    }

    message = protocolClient->PopResponse();
    command = &message->_Data;
    ASSERT_EQ(CustomProtocolCommand::Code::BenchmarkEnd, command->Command);
    ASSERT_EQ(loopCount, command->Payload.BenchmarkPayload.Response.NopCount);

    GOUT(loopCount << " NOPs took " << command->Payload.BenchmarkPayload.Response.Duration << " ms to process");

    std::shared_ptr<MessageClient<SimFrameworkCommand>> client = std::make_shared<MessageClient<SimFrameworkCommand>>(SSDSIM_IPC_NAME);
    Message<SimFrameworkCommand> *simMessage = allocateSimFrameworkCommand(client, SimFrameworkCommand::Code::Exit);
    client->Push(simMessage);
}

TEST(MessagingSystem, NopMessageWithResponse)
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