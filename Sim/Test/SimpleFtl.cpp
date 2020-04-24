#include "pch.h"

#define NOMINMAX
#include <Windows.h>

#include "Test/gtest-cout.h"

#include "SimFramework/Framework.h"

#include "HostComm.hpp"
#include "SimpleFtl/Translation.h"
#include "SsdSimApp.h"

using namespace HostCommTest;

void SetReadWriteCommand(CustomProtocolCommand &command, CustomProtocolCommand::Code code, const U32 &lba, const U32 &sectorCount)
{
    command.Command = code;
    command.Descriptor.SimpleFtlPayload.Lba = lba;
    command.Descriptor.SimpleFtlPayload.SectorCount = sectorCount;
}

U32 GetSectorSizeInTransfer(const DeviceInfoPayload &deviceInfo)
{
    return deviceInfo.SectorInfo.CompactMode
        ? deviceInfo.SectorInfo.CompactSizeInByte : (1 << deviceInfo.SectorInfo.SectorSizeInBit);
}

void VerifyLbaToNand(const U32 &lba, const U32 &sectorCount,
    const NandHal::NandAddress &expectedAddress, const U32 &expectedLba, const U32 &expectedSectorCount)
{
    NandHal::NandAddress address;
    U32 nextLba;
    U32 remainingSector;
    SimpleFtlTranslation::LbaToNandAddress(lba, sectorCount, address, nextLba, remainingSector);

    ASSERT_EQ(address.Channel._, expectedAddress.Channel._);
    ASSERT_EQ(address.Device._, expectedAddress.Device._);
    ASSERT_EQ(address.Block._, expectedAddress.Block._);
    ASSERT_EQ(address.Page._, expectedAddress.Page._);
    ASSERT_EQ(address.Sector._, expectedAddress.Sector._);
    ASSERT_EQ(address.SectorCount._, expectedAddress.SectorCount._);
    ASSERT_EQ(nextLba, expectedLba);
    ASSERT_EQ(remainingSector, expectedSectorCount);
}

class SimpleFtlTest : public ::testing::Test
{
protected:
	void SetUp() override 
	{
		ASSERT_NO_THROW(SimFramework.Init("Hardwareconfig/hardwaremin.json"));

		FrameworkFuture = std::async(std::launch::async, &(Framework::operator()), &SimFramework);

		std::this_thread::sleep_for(std::chrono::milliseconds(1000));

		// TODO: implement way to get server name
		constexpr char* customMessagingName = "SsdSimCustomProtocolServer";
		CustomProtocolClient = std::make_shared<MessageClient<CustomProtocolCommand>>(customMessagingName);
		ASSERT_NE(nullptr, CustomProtocolClient);

		// Load dll SimpleFtl by sending command DownloadAndExecute
		constexpr char* simpleFtlDll = "SimpleFtl.dll";
		auto downloadAndExecuteMsg = AllocateMessage<CustomProtocolCommand>(CustomProtocolClient, 0, false);
		ASSERT_NE(downloadAndExecuteMsg, nullptr);
        downloadAndExecuteMsg->Data.Command = CustomProtocolCommand::Code::DownloadAndExecute;
		memcpy(downloadAndExecuteMsg->Data.Descriptor.DownloadAndExecute.CodeName, simpleFtlDll,
			sizeof(downloadAndExecuteMsg->Data.Descriptor.DownloadAndExecute.CodeName));
		CustomProtocolClient->Push(downloadAndExecuteMsg);

        // Set user sector size and enable compact mode
        auto setSectorSizeMsg = AllocateMessage<CustomProtocolCommand>(CustomProtocolClient, 0, true);
        ASSERT_NE(setSectorSizeMsg, nullptr);
        setSectorSizeMsg->Data.Command = CustomProtocolCommand::Code::SetSectorSize;
        setSectorSizeMsg->Data.Descriptor.SectorInfoPayload.SectorInfo.SectorSizeInBit = 9;
        setSectorSizeMsg->Data.Descriptor.SectorInfoPayload.SectorInfo.CompactMode = true;
        setSectorSizeMsg->Data.Descriptor.SectorInfoPayload.SectorInfo.CompactSizeInByte = 30;
        CustomProtocolClient->Push(setSectorSizeMsg);
        while (!CustomProtocolClient->HasResponse());
        auto setSectorSizeRsp = CustomProtocolClient->PopResponse();
        ASSERT_EQ(CustomProtocolCommand::Status::Success, setSectorSizeRsp->Data.CommandStatus);

		// Get device info
		auto getDeviceInfoMsg = AllocateMessage<CustomProtocolCommand>(CustomProtocolClient, 0, true);
		ASSERT_NE(getDeviceInfoMsg, nullptr);
        getDeviceInfoMsg->Data.Command = CustomProtocolCommand::Code::GetDeviceInfo;
		CustomProtocolClient->Push(getDeviceInfoMsg);
		while (!CustomProtocolClient->HasResponse());
        CustomProtocolMessage* deviceInfoResponse = CustomProtocolClient->PopResponse();
		ASSERT_EQ(CustomProtocolCommand::Status::Success, deviceInfoResponse->Data.CommandStatus);
        DeviceInfo = deviceInfoResponse->Data.Descriptor.DeviceInfoPayload;
        CustomProtocolClient->DeallocateMessage(deviceInfoResponse);
        SectorSizeInTransfer = GetSectorSizeInTransfer(DeviceInfo);
	}

	void TearDown() override
	{
		//TODO: implement way to get server name
		constexpr char* messagingName = "SsdSimMainMessageServer";	//TODO: define a way to get name
		auto client = std::make_shared<SimFrameworkMessageClient>(messagingName);
		ASSERT_NE(nullptr, client);

		auto message = AllocateMessage<SimFrameworkCommand>(client, 0, false);
		ASSERT_NE(message, nullptr);
		message->Data.Code = SimFrameworkCommand::Code::Exit;
		client->Push(message);

		//Give the Framework a chance to stop completely before next test
		// This is a work around until multiple servers can be created without collision
		std::this_thread::sleep_for(std::chrono::milliseconds(3000));
	}

	Framework SimFramework;
	std::future<void> FrameworkFuture;
	CustomProtocolMessageClientSharedPtr CustomProtocolClient;
    DeviceInfoPayload DeviceInfo;
    U32 SectorSizeInTransfer;
};

TEST(SimpleFtl, Translation_LbaToNand)
{
    constexpr U8 SectorSizeInBit = 9;

    NandHal::Geometry geometry;
    geometry.ChannelCount = 4;
    geometry.DevicesPerChannel = 2;
    geometry.BlocksPerDevice = 128;
    geometry.PagesPerBlock = 256;
    geometry.BytesPerPage = 8192;

    U32 sectorsPerPage = geometry.BytesPerPage >> SectorSizeInBit;
    U32 nextLba, remainingSector;

    U32 lba = 0;
    NandHal::NandAddress address;

    SimpleFtlTranslation::SetGeometry(geometry);
    SimpleFtlTranslation::SetSectorSize(SectorSizeInBit);
    for (U32 block(0); block < geometry.BlocksPerDevice; ++block)
    {
        for (U32 page(0); page < geometry.PagesPerBlock; ++page)
        {
            for (U32 device(0); device < geometry.DevicesPerChannel; ++device)
            {
                for (U32 channel(0); channel < geometry.ChannelCount; ++channel)
                {
                    SimpleFtlTranslation::LbaToNandAddress(lba, sectorsPerPage, address, nextLba, remainingSector);
                    ASSERT_EQ(address.Channel._, channel);
                    ASSERT_EQ(address.Device._, device);
                    ASSERT_EQ(address.Page._, page);
                    ASSERT_EQ(address.Block._, block);
                    ASSERT_EQ(address.Sector._, 0);
                    ASSERT_EQ(address.SectorCount._, sectorsPerPage);
                    lba += (geometry.BytesPerPage >> SectorSizeInBit);
                    ASSERT_EQ(lba, nextLba);
                    ASSERT_EQ(remainingSector, 0);
                }
            }
        }
    }

    // Unaligned lba and sector count
    NandHal::NandAddress expectedAddress;
    expectedAddress.Channel._ = 0;
    expectedAddress.Device._ = 0;
    expectedAddress.Block._ = 0;
    expectedAddress.Page._ = 0;
    expectedAddress.Sector._ = 0;
    expectedAddress.SectorCount._ = 0;

    expectedAddress.Sector._ = 1;
    expectedAddress.SectorCount._ = sectorsPerPage - 2;
    VerifyLbaToNand(1, sectorsPerPage - 2, expectedAddress, sectorsPerPage - 1, 0);

    expectedAddress.Sector._ = 1;
    expectedAddress.SectorCount._ = sectorsPerPage - 1;
    VerifyLbaToNand(1, sectorsPerPage, expectedAddress, sectorsPerPage, 1);

    expectedAddress.Sector._ = 1;
    expectedAddress.SectorCount._ = sectorsPerPage - 1;
    VerifyLbaToNand(1, 2 * sectorsPerPage - 2, expectedAddress, sectorsPerPage, sectorsPerPage - 1);

    expectedAddress.Sector._ = 0;
    expectedAddress.SectorCount._ = sectorsPerPage - 1;
    VerifyLbaToNand(0, sectorsPerPage - 1, expectedAddress, sectorsPerPage - 1, 0);

    expectedAddress.Sector._ = 0;
    expectedAddress.SectorCount._ = sectorsPerPage;
    VerifyLbaToNand(0, 2 * sectorsPerPage - 1, expectedAddress, sectorsPerPage, sectorsPerPage - 1);
}

TEST(SimpleFtl, BasicWriteReadVerify_App)
{
    //Start the app
    GOUT("Starting SsdSim process.");
    SHELLEXECUTEINFO ShExecInfo = { 0 };
    ASSERT_TRUE(LaunchProcess("SsdSim.exe", "--hardwarespec Hardwareconfig/hardwarespec.json", ShExecInfo));

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    DWORD exitCode = 0;
    ASSERT_TRUE(GetExitCodeProcess(ShExecInfo.hProcess, &exitCode));
    ASSERT_EQ(STILL_ACTIVE, exitCode);

    //TODO: implement way to get server name
    constexpr char* customMessagingName = "SsdSimCustomProtocolServer";
    auto clientCustomProtocolCmd = std::make_shared<MessageClient<CustomProtocolCommand>>(customMessagingName);
    ASSERT_NE(nullptr, clientCustomProtocolCmd);

    //Load dll SimpleFtl by sending command DownloadAndExecute
    constexpr char* simpleFtlDll = "SimpleFtl.dll";
    auto messageDownloadAndExecute = AllocateMessage<CustomProtocolCommand>(clientCustomProtocolCmd, 0, false);
    ASSERT_NE(messageDownloadAndExecute, nullptr);
    messageDownloadAndExecute->Data.Command = CustomProtocolCommand::Code::DownloadAndExecute;
    memcpy(messageDownloadAndExecute->Data.Descriptor.DownloadAndExecute.CodeName, simpleFtlDll,
        sizeof(messageDownloadAndExecute->Data.Descriptor.DownloadAndExecute.CodeName));
    clientCustomProtocolCmd->Push(messageDownloadAndExecute);

    //Get device info
    auto messageGetDeviceInfo = AllocateMessage<CustomProtocolCommand>(clientCustomProtocolCmd, 0, true);
    ASSERT_NE(messageGetDeviceInfo, nullptr);
    messageGetDeviceInfo->Data.Command = CustomProtocolCommand::Code::GetDeviceInfo;
    clientCustomProtocolCmd->Push(messageGetDeviceInfo);

    while (!clientCustomProtocolCmd->HasResponse());
    auto responseGetDeviceInfo = clientCustomProtocolCmd->PopResponse();
	ASSERT_EQ(CustomProtocolCommand::Status::Success, responseGetDeviceInfo->Data.CommandStatus);

    //Write a buffer with lba and sector count
    constexpr U32 lba = 123455;
    U32 sectorCount = 35;
    U32 sectorSizeInTransfer = GetSectorSizeInTransfer(responseGetDeviceInfo->Data.Descriptor.DeviceInfoPayload);
    U32 payloadSize = sectorCount * sectorSizeInTransfer;
    auto writeMessage = AllocateMessage<CustomProtocolCommand>(clientCustomProtocolCmd, payloadSize, true);
    ASSERT_NE(writeMessage, nullptr);
    ASSERT_NE(writeMessage->Payload, nullptr);
    for (auto i(0); i < sectorCount; ++i)
    {
        auto buffer = &(static_cast<U8*>(writeMessage->Payload)[i * sectorSizeInTransfer]);
        memset((void*)buffer, lba + i, sectorSizeInTransfer);
    }
    SetReadWriteCommand(writeMessage->Data, CustomProtocolCommand::Code::Write, lba, sectorCount);
    clientCustomProtocolCmd->Push(writeMessage);

    //Wait for write command response
    while (!clientCustomProtocolCmd->HasResponse());
    auto responseMessageWrite = clientCustomProtocolCmd->PopResponse();
	ASSERT_EQ(CustomProtocolCommand::Status::Success, responseMessageWrite->Data.CommandStatus);

    //Read a buffer with lba and sector count
    auto readMessage = AllocateMessage<CustomProtocolCommand>(clientCustomProtocolCmd, payloadSize, true);
    ASSERT_NE(readMessage, nullptr);
    SetReadWriteCommand(readMessage->Data, CustomProtocolCommand::Code::Read, lba, sectorCount);
    clientCustomProtocolCmd->Push(readMessage);

    //Wait for read command response
    while (!clientCustomProtocolCmd->HasResponse());
    auto responseMessageRead = clientCustomProtocolCmd->PopResponse();
	ASSERT_EQ(CustomProtocolCommand::Status::Success, responseMessageRead->Data.CommandStatus);

    //Get message read buffer to verify with the write buffer
    int compareResult = std::memcmp(responseMessageWrite->Payload, responseMessageRead->Payload, payloadSize);

    //--Deallocate
    clientCustomProtocolCmd->DeallocateMessage(responseGetDeviceInfo);
    clientCustomProtocolCmd->DeallocateMessage(responseMessageWrite);
    clientCustomProtocolCmd->DeallocateMessage(responseMessageRead);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    //TODO: implement way to get server name
    constexpr char* messagingName = "SsdSimMainMessageServer";	//TODO: define a way to get name
    auto client = std::make_shared<MessageClient<SimFrameworkCommand>>(messagingName);
    ASSERT_NE(nullptr, client);

    auto message = AllocateMessage<SimFrameworkCommand>(client, 0, false);
    ASSERT_NE(message, nullptr);
    message->Data.Code = SimFrameworkCommand::Code::Exit;
    client->Push(message);

    if (ShExecInfo.hProcess)
    {
        GOUT("Waiting for process termination.");
        ASSERT_NE(WAIT_TIMEOUT, WaitForSingleObject(ShExecInfo.hProcess, 10000));
        GetExitCodeProcess(ShExecInfo.hProcess, &exitCode);
        CloseHandle(ShExecInfo.hProcess);
    }

    ASSERT_EQ(0, compareResult);
}

TEST_F(SimpleFtlTest, BasicWriteReadVerify)
{
	U32 totalSector = DeviceInfo.TotalSector;

	//Write a buffer with lba and sector count
    constexpr U32 lba = 12345;
    U32 sectorCount = 25;
    U32 payloadSize = sectorCount * SectorSizeInTransfer;
    auto writeMessage = AllocateMessage<CustomProtocolCommand>(CustomProtocolClient, payloadSize, true);
    ASSERT_NE(writeMessage, nullptr);
    ASSERT_NE(writeMessage->Payload, nullptr);
    for (auto i(0); i < sectorCount; ++i)
    {
        auto buffer = &(static_cast<U8*>(writeMessage->Payload)[i * SectorSizeInTransfer]);
        memset((void*)buffer, lba + i, SectorSizeInTransfer);
    }
    SetReadWriteCommand(writeMessage->Data, CustomProtocolCommand::Code::Write, lba, sectorCount);
	CustomProtocolClient->Push(writeMessage);

    //Wait for write command response
    while (!CustomProtocolClient->HasResponse());
    auto responseMessageWrite = CustomProtocolClient->PopResponse();
	ASSERT_EQ(CustomProtocolCommand::Status::Success, responseMessageWrite->Data.CommandStatus);

	//Read a buffer with lba and sector count
    auto readMessage = AllocateMessage<CustomProtocolCommand>(CustomProtocolClient, payloadSize, true);
    ASSERT_NE(readMessage, nullptr);
    SetReadWriteCommand(readMessage->Data, CustomProtocolCommand::Code::Read, lba, sectorCount);
    CustomProtocolClient->Push(readMessage);

    //Wait for read command response
    while (!CustomProtocolClient->HasResponse());
    auto responseMessageRead = CustomProtocolClient->PopResponse();
	ASSERT_EQ(CustomProtocolCommand::Status::Success, responseMessageRead->Data.CommandStatus);

    //Get message read buffer to verify with the write buffer
    int compareResult = std::memcmp(responseMessageWrite->Payload, responseMessageRead->Payload, payloadSize);
    if (compareResult != 0)
    {
        for (auto i(0); i < sectorCount; ++i)
        {
            auto writeBuffer = &(static_cast<U8*>(responseMessageWrite->Payload)[i * SectorSizeInTransfer]);
            auto readBuffer = &(static_cast<U8*>(responseMessageRead->Payload)[i * SectorSizeInTransfer]);
            auto result = std::memcmp(writeBuffer, readBuffer, SectorSizeInTransfer);
            if (result != 0)
            {
                GOUT("Miscompare at sector " << i);
            }
        }
        FAIL();
    }

    //--Deallocate
	CustomProtocolClient->DeallocateMessage(responseMessageWrite);
	CustomProtocolClient->DeallocateMessage(responseMessageRead);
}

TEST_F(SimpleFtlTest, BasicAscendingWriteReadVerifyAll)
{
    U32 totalSector = DeviceInfo.TotalSector;

    constexpr U32 maxSectorPerTransfer = 256;
    U32 payloadSize = maxSectorPerTransfer * SectorSizeInTransfer;
    auto writeMessage = AllocateMessage<CustomProtocolCommand>(CustomProtocolClient, payloadSize, true);
    ASSERT_NE(writeMessage, nullptr);
    ASSERT_NE(writeMessage->Payload, nullptr);
    auto readMessage = AllocateMessage<CustomProtocolCommand>(CustomProtocolClient, payloadSize, true);
    ASSERT_NE(readMessage, nullptr);

    //Write and read to verify all written data in ascending order
    U32 sectorCount;
    U8 pattern = 0;
    for (U32 lba(0); lba < totalSector; lba += maxSectorPerTransfer)
    {
        sectorCount = std::min(maxSectorPerTransfer, totalSector - lba);
        //Fill write buffer
        memset(writeMessage->Payload, pattern, sectorCount * SectorSizeInTransfer);
        ++pattern;
        SetReadWriteCommand(writeMessage->Data, CustomProtocolCommand::Code::Write, lba, sectorCount);
        CustomProtocolClient->Push(writeMessage);

        //Wait for write command response
        while (!CustomProtocolClient->HasResponse());
        auto responseMessageWrite = CustomProtocolClient->PopResponse();
		ASSERT_EQ(CustomProtocolCommand::Status::Success, responseMessageWrite->Data.CommandStatus);

        SetReadWriteCommand(readMessage->Data, CustomProtocolCommand::Code::Read, lba, sectorCount);
        CustomProtocolClient->Push(readMessage);

        //Wait for read command response
        while (!CustomProtocolClient->HasResponse());
        auto responseMessageRead = CustomProtocolClient->PopResponse();
		ASSERT_EQ(CustomProtocolCommand::Status::Success, responseMessageRead->Data.CommandStatus);

        //Get message read buffer to verify with the write buffer
        auto pReadData = CustomProtocolClient->GetMessage(responseMessageRead->Id())->Payload;
        auto result = std::memcmp(responseMessageWrite->Payload, pReadData,
            sectorCount * SectorSizeInTransfer);
        ASSERT_EQ(0, result);
    }
}

TEST_F(SimpleFtlTest, BasicDescendingWriteReadVerifyAll)
{
    U32 totalSector = DeviceInfo.TotalSector;

    constexpr U32 maxSectorPerTransfer = 256;
    U32 payloadSize = maxSectorPerTransfer * SectorSizeInTransfer;
    auto writeMessage = AllocateMessage<CustomProtocolCommand>(CustomProtocolClient, payloadSize, true);
    ASSERT_NE(writeMessage, nullptr);
    ASSERT_NE(writeMessage->Payload, nullptr);
    auto readMessage = AllocateMessage<CustomProtocolCommand>(CustomProtocolClient, payloadSize, true);
    ASSERT_NE(readMessage, nullptr);

    //Write and read to verify all written data in descending order
    U32 sectorCount;
    U8 pattern = 0;
    for (U32 lba(totalSector); lba > 0; )
    {
        if (lba > maxSectorPerTransfer)
        {
            lba = lba - maxSectorPerTransfer;
            sectorCount = maxSectorPerTransfer;
        }
        else
        {
            sectorCount = lba;
            lba = 0;
        }
        //Fill write buffer
        memset(writeMessage->Payload, pattern, payloadSize);
        ++pattern;
        SetReadWriteCommand(writeMessage->Data, CustomProtocolCommand::Code::Write, lba, sectorCount);
        CustomProtocolClient->Push(writeMessage);

        //Wait for write command response
        while (!CustomProtocolClient->HasResponse());
        auto responseMessageWrite = CustomProtocolClient->PopResponse();
		ASSERT_EQ(CustomProtocolCommand::Status::Success, responseMessageWrite->Data.CommandStatus);

        SetReadWriteCommand(readMessage->Data, CustomProtocolCommand::Code::Read, lba, sectorCount);
        CustomProtocolClient->Push(readMessage);

        //Wait for read command response
        while (!CustomProtocolClient->HasResponse());
        auto responseMessageRead = CustomProtocolClient->PopResponse();
		ASSERT_EQ(CustomProtocolCommand::Status::Success, responseMessageRead->Data.CommandStatus);

        //Get message read buffer to verify with the write buffer
        auto pReadData = CustomProtocolClient->GetMessage(responseMessageRead->Id())->Payload;
        auto result = std::memcmp(responseMessageWrite->Payload, pReadData,
            sectorCount * SectorSizeInTransfer);
        ASSERT_EQ(0, result);
    }
}

TEST_F(SimpleFtlTest, BasicWriteReadBenchmark)
{
    using namespace std::chrono;

    constexpr U32 commandCount = 10;
    constexpr U32 lba = 0;
    constexpr U32 sectorCount = 256;
    U32 payloadSize = sectorCount * SectorSizeInTransfer;
    Message<CustomProtocolCommand>* messages[commandCount];
    for (auto i = 0; i < commandCount; ++i)
    {
        messages[i] = AllocateMessage<CustomProtocolCommand>(CustomProtocolClient, payloadSize, true);
        ASSERT_NE(messages[i], nullptr);
    }

    unsigned long totalBytesWrittenInSeconds = 0;
    double writeRate = 0;
    high_resolution_clock::time_point t0;
    for (auto i = 0; i < commandCount; ++i)
    {
        SetReadWriteCommand(messages[i]->Data, CustomProtocolCommand::Code::Write, lba + (i * sectorCount), sectorCount);
        memset(messages[i]->Payload, 0xaa, messages[i]->PayloadSize);
    }

    t0 = high_resolution_clock::now();
    for (auto i = 0; i < commandCount; ++i)
    {
        CustomProtocolClient->Push(messages[i]);
    }

    U32 responseReceivedCount = 0;
    while (responseReceivedCount < commandCount)
    {
        if (CustomProtocolClient->HasResponse())
        {
            messages[responseReceivedCount] = CustomProtocolClient->PopResponse();
            totalBytesWrittenInSeconds += messages[responseReceivedCount]->Data.Descriptor.SimpleFtlPayload.SectorCount << DeviceInfo.SectorInfo.SectorSizeInBit;
            responseReceivedCount++;
        }
    }
    auto t1 = high_resolution_clock::now();
    auto delta = duration<double>(t1 - t0);
    writeRate = (double)(totalBytesWrittenInSeconds / 1024 / 1024) / delta.count();

    //--Read in benchmark
    unsigned long totalBytesReadInSeconds = 0;
    double readRate = 0;
    for (auto i = 0; i < commandCount; ++i)
    {
        SetReadWriteCommand(messages[i]->Data, CustomProtocolCommand::Code::Read, lba + (i * sectorCount), sectorCount);
    }

    t0 = high_resolution_clock::now();;
    for (auto i = 0; i < commandCount; ++i)
    {
        CustomProtocolClient->Push(messages[i]);
    }

    responseReceivedCount = 0;
    while (responseReceivedCount < commandCount)
    {
        if (CustomProtocolClient->HasResponse())
        {
            messages[responseReceivedCount] = CustomProtocolClient->PopResponse();
            totalBytesReadInSeconds += messages[responseReceivedCount]->Data.Descriptor.SimpleFtlPayload.SectorCount << DeviceInfo.SectorInfo.SectorSizeInBit;
            responseReceivedCount++;
        }
    }

    t1 = high_resolution_clock::now();
    delta = duration<double>(t1 - t0);
    readRate = (double)(totalBytesReadInSeconds / 1024 / 1024) / delta.count();

    GOUT("Write/Read benchmark");
    GOUT("   Write rate: " << writeRate << " MiB/s");
    GOUT("   Read rate: " << readRate << " MiB/s");

    //--Deallocate
    for (auto i = 0; i < commandCount; ++i)
    {
        CustomProtocolClient->DeallocateMessage(messages[i]);
    }
}

TEST_F(SimpleFtlTest, BasicUnalignedWriteAlignedRead)
{
	U32 bytesPerSector = SectorSizeInTransfer;
	U8 sectorsPerPage = DeviceInfo.SectorsPerPage;

	ASSERT_EQ(sectorsPerPage >= 4, true);	//this is for this specific test setup

	U32 lba = sectorsPerPage / 2;
	U32 sectorCount = sectorsPerPage / 4;

	auto writeMessage = AllocateMessage<CustomProtocolCommand>(CustomProtocolClient, sectorCount * bytesPerSector, true);
	ASSERT_NE(writeMessage, nullptr);
	ASSERT_NE(writeMessage->Payload, nullptr);
    SetReadWriteCommand(writeMessage->Data, CustomProtocolCommand::Code::Write, lba, sectorCount);
	for (auto i(0); i < sectorCount; ++i)
	{
		auto buffer = &(static_cast<U8*>(writeMessage->Payload)[i * bytesPerSector]);
		memset((void*)buffer, lba + i, bytesPerSector);
	}
	CustomProtocolClient->Push(writeMessage);
	while (!CustomProtocolClient->HasResponse());
	auto writeMessageReponse = CustomProtocolClient->PopResponse();
	ASSERT_EQ(writeMessage, writeMessageReponse);
	ASSERT_EQ(CustomProtocolCommand::Status::Success, writeMessageReponse->Data.CommandStatus);

	auto readMessage = AllocateMessage<CustomProtocolCommand>(CustomProtocolClient, bytesPerSector * sectorsPerPage, true);
	ASSERT_NE(readMessage, nullptr);
    SetReadWriteCommand(readMessage->Data, CustomProtocolCommand::Code::Read, 0, sectorsPerPage);
	CustomProtocolClient->Push(readMessage);
    while (!CustomProtocolClient->HasResponse());
	auto readMessageReponse = CustomProtocolClient->PopResponse();
	ASSERT_EQ(readMessage, readMessageReponse);
	ASSERT_EQ(CustomProtocolCommand::Status::Success, readMessageReponse->Data.CommandStatus);

	auto writeMessageBuffer = static_cast<void*>(writeMessageReponse->Payload);
	auto readMessageBuffer = &(static_cast<U8*>(readMessageReponse->Payload)[lba * bytesPerSector]);
	auto result = memcmp((void*)readMessageBuffer, writeMessageBuffer, sectorCount * bytesPerSector);
	ASSERT_EQ(0, result);
}

//! Sends the following IO command sequence
/*
	Write 0:256
	Read verify 0:256
	Write 0:256
	Read verify 0:256
*/
TEST_F(SimpleFtlTest, BasicRepeatedWriteReadVerify)
{
	constexpr U32 lba = 0;
	constexpr U32 sectorCount = 256;
	U32 bytesPerSector = SectorSizeInTransfer;
	U32 payloadSize = sectorCount * bytesPerSector;

	ASSERT_EQ(DeviceInfo.TotalSector >= sectorCount, true);

	auto writeMessage = AllocateMessage<CustomProtocolCommand>(CustomProtocolClient, payloadSize, true);
	ASSERT_NE(writeMessage, nullptr);
	ASSERT_NE(writeMessage->Payload, nullptr);
    SetReadWriteCommand(writeMessage->Data, CustomProtocolCommand::Code::Write, lba, sectorCount);
	CustomProtocolMessage* writeMessageReponse;

	auto readMessage = AllocateMessage<CustomProtocolCommand>(CustomProtocolClient, payloadSize, true);
	ASSERT_NE(readMessage, nullptr);
    SetReadWriteCommand(readMessage->Data, CustomProtocolCommand::Code::Read, lba, sectorCount);
	CustomProtocolMessage* readMessageReponse;

	constexpr auto repeatCount = 2;
	U8 bytePattern[repeatCount] = { 0xa5, 0xff };
	for (auto loop(0); loop < repeatCount; ++loop)
	{
		for (U32 i(0); i < payloadSize; ++i)
		{
			((U8*)writeMessage->Payload)[i] = bytePattern[loop];
		}

		CustomProtocolClient->Push(writeMessage);
		while (!CustomProtocolClient->HasResponse());
		writeMessageReponse = CustomProtocolClient->PopResponse();
		ASSERT_EQ(writeMessage, writeMessageReponse);
		ASSERT_EQ(CustomProtocolCommand::Status::Success, writeMessageReponse->Data.CommandStatus);

		CustomProtocolClient->Push(readMessage);
		while (!CustomProtocolClient->HasResponse());
		readMessageReponse = CustomProtocolClient->PopResponse();
		ASSERT_EQ(readMessage, readMessageReponse);
		
		ASSERT_EQ(CustomProtocolCommand::Status::Success, readMessageReponse->Data.CommandStatus);
		auto result = std::memcmp(writeMessage->Payload, readMessageReponse->Payload, payloadSize);
		ASSERT_EQ(0, result);
	}

	CustomProtocolClient->DeallocateMessage(writeMessage);
	CustomProtocolClient->DeallocateMessage(readMessage);
}