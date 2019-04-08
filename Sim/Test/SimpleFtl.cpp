#include "pch.h"

#define NOMINMAX
#include <Windows.h>

#include "Test/gtest-cout.h"

#include "SimFramework/Framework.h"

#include "HostComm.hpp"
#include "SimpleFtl/Translation.h"
#include "SsdSimApp.h"

using namespace HostCommTest;

TEST(SimpleFtl, Translation_LbaToNand)
{
    NandHal::Geometry geometry;
    geometry._ChannelCount = 4;
    geometry._DevicesPerChannel = 2;
    geometry._BlocksPerDevice = 128;
    geometry._PagesPerBlock = 256;
    geometry._BytesPerPage = 8192;

    U32 lba = 0;
    NandHal::CommandDesc cmdDesc;

    for (U32 block(0); block < geometry._BlocksPerDevice; ++block)
    {
        for (U32 page(0); page < geometry._PagesPerBlock; ++page)
        {
            for (U32 device(0); device < geometry._DevicesPerChannel; ++device)
            {
                for (U32 channel(0); channel < geometry._ChannelCount; ++channel)
                {
                    SimpleFtlTranslation::LbaToNandAddress(geometry, lba, cmdDesc);
                    ASSERT_EQ(cmdDesc.Channel._, channel);
                    ASSERT_EQ(cmdDesc.Device._, device);
                    ASSERT_EQ(cmdDesc.Page._, page);
                    ASSERT_EQ(cmdDesc.Block._, block);
                    lba += (geometry._BytesPerPage >> 9);
                }
            }
        }
    }
}

TEST(SimpleFtl, Basic_WriteReadVerify)
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
    messageDownloadAndExecute->_Data.Command = CustomProtocolCommand::Code::DownloadAndExecute;
    memcpy(messageDownloadAndExecute->_Data.Descriptor.DownloadAndExecute.CodeName, simpleFtlDll,
        sizeof(messageDownloadAndExecute->_Data.Descriptor.DownloadAndExecute.CodeName));
    clientCustomProtocolCmd->Push(messageDownloadAndExecute);

    //Get device info
    auto messageGetDeviceInfo = AllocateMessage<CustomProtocolCommand>(clientCustomProtocolCmd, 0, true);
    ASSERT_NE(messageGetDeviceInfo, nullptr);
    messageGetDeviceInfo->_Data.Command = CustomProtocolCommand::Code::GetDeviceInfo;
    clientCustomProtocolCmd->Push(messageGetDeviceInfo);

    while (!clientCustomProtocolCmd->HasResponse());
    auto responseGetDeviceInfo = clientCustomProtocolCmd->PopResponse();

    //Write a buffer with lba and sector count
    constexpr U32 lba = 123456;
    constexpr U32 sectorCount = 100;
    U32 payloadSize = sectorCount * responseGetDeviceInfo->_Data.Descriptor.DeviceInfoPayload.BytesPerSector;
    auto messageWrite = AllocateMessage<CustomProtocolCommand>(clientCustomProtocolCmd, payloadSize, true);
    ASSERT_NE(messageWrite, nullptr);
    ASSERT_NE(messageWrite->_Payload, nullptr);
    for (U32 i(0); i < payloadSize; ++i)
    {
        ((U8*)messageWrite->_Payload)[i] = i % 255;
    }
    messageWrite->_Data.Command = CustomProtocolCommand::Code::Write;
    messageWrite->_Data.Descriptor.SimpleFtlPayload.Lba = lba;
    messageWrite->_Data.Descriptor.SimpleFtlPayload.SectorCount = sectorCount;
    clientCustomProtocolCmd->Push(messageWrite);

    //Wait for write command response
    while (!clientCustomProtocolCmd->HasResponse());
    auto responseMessageWrite = clientCustomProtocolCmd->PopResponse();

    //Read a buffer with lba and sector count
    auto messageRead = AllocateMessage<CustomProtocolCommand>(clientCustomProtocolCmd, payloadSize, true);
    ASSERT_NE(messageRead, nullptr);
    messageRead->_Data.Command = CustomProtocolCommand::Code::Read;
    messageRead->_Data.Descriptor.SimpleFtlPayload.Lba = lba;
    messageRead->_Data.Descriptor.SimpleFtlPayload.SectorCount = sectorCount;
    clientCustomProtocolCmd->Push(messageRead);

    //Wait for read command response
    while (!clientCustomProtocolCmd->HasResponse());
    auto responseMessageRead = clientCustomProtocolCmd->PopResponse();

    //Get message read buffer to verify with the write buffer
    auto result = std::memcmp(responseMessageWrite->_Payload, responseMessageRead->_Payload, payloadSize);

    ASSERT_EQ(0, result);

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
    message->_Data.Code = SimFrameworkCommand::Code::Exit;
    client->Push(message);

    if (ShExecInfo.hProcess)
    {
        GOUT("Waiting for process termination.");
        ASSERT_NE(WAIT_TIMEOUT, WaitForSingleObject(ShExecInfo.hProcess, 10000));
        GetExitCodeProcess(ShExecInfo.hProcess, &exitCode);
        CloseHandle(ShExecInfo.hProcess);
    }
}

TEST(SimpleFtl, Basic_AscendingWriteReadVerifyAll)
{
    Framework framework;
    ASSERT_NO_THROW(framework.Init("Hardwareconfig/hardwaremin.json"));

    auto fwFuture = std::async(std::launch::async, &(Framework::operator()), &framework);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    //TODO: implement way to get server name
    constexpr char* customMessagingName = "SsdSimCustomProtocolServer";
    auto clientCustomProtocolCmd = std::make_shared<MessageClient<CustomProtocolCommand>>(customMessagingName);
    ASSERT_NE(nullptr, clientCustomProtocolCmd);

    //Load dll SimpleFtl by sending command DownloadAndExecute
    constexpr char* simpleFtlDll = "SimpleFtl.dll";
    auto messageDownloadAndExecute = AllocateMessage<CustomProtocolCommand>(clientCustomProtocolCmd, 0, false);
    ASSERT_NE(messageDownloadAndExecute, nullptr);
    messageDownloadAndExecute->_Data.Command = CustomProtocolCommand::Code::DownloadAndExecute;
    memcpy(messageDownloadAndExecute->_Data.Descriptor.DownloadAndExecute.CodeName, simpleFtlDll,
        sizeof(messageDownloadAndExecute->_Data.Descriptor.DownloadAndExecute.CodeName));
    clientCustomProtocolCmd->Push(messageDownloadAndExecute);

    //Get device info to get lba count
    auto messageGetDeviceInfo = AllocateMessage<CustomProtocolCommand>(clientCustomProtocolCmd, 0, true);
    ASSERT_NE(messageGetDeviceInfo, nullptr);
    messageGetDeviceInfo->_Data.Command = CustomProtocolCommand::Code::GetDeviceInfo;
    clientCustomProtocolCmd->Push(messageGetDeviceInfo);

    while (!clientCustomProtocolCmd->HasResponse());
    auto responseGetDeviceInfo = clientCustomProtocolCmd->PopResponse();
    U32 lbaCount = responseGetDeviceInfo->_Data.Descriptor.DeviceInfoPayload.LbaCount;

    constexpr U32 maxSectorPerTransfer = 256;
    U32 payloadSize = maxSectorPerTransfer * responseGetDeviceInfo->_Data.Descriptor.DeviceInfoPayload.BytesPerSector;
    auto messageWrite = AllocateMessage<CustomProtocolCommand>(clientCustomProtocolCmd, payloadSize, true);
    ASSERT_NE(messageWrite, nullptr);
    ASSERT_NE(messageWrite->_Payload, nullptr);
    auto messageRead = AllocateMessage<CustomProtocolCommand>(clientCustomProtocolCmd, payloadSize, true);
    ASSERT_NE(messageRead, nullptr);

    //Write and read to verify all written data in ascending order
    U32 sectorCount;
    for (U32 lba(0); lba < lbaCount; lba += maxSectorPerTransfer)
    {
        sectorCount = std::min(maxSectorPerTransfer, lbaCount - lba);
        //Fill write buffer
        for (U32 i(0); i < payloadSize; ++i)
        {
            ((U8*)messageWrite->_Payload)[i] = i % 255;
        }
        messageWrite->_Data.Command = CustomProtocolCommand::Code::Write;
        messageWrite->_Data.Descriptor.SimpleFtlPayload.Lba = lba;
        messageWrite->_Data.Descriptor.SimpleFtlPayload.SectorCount = sectorCount;
        clientCustomProtocolCmd->Push(messageWrite);

        //Wait for write command response
        while (!clientCustomProtocolCmd->HasResponse());
        auto responseMessageWrite = clientCustomProtocolCmd->PopResponse();

        messageRead->_Data.Command = CustomProtocolCommand::Code::Read;
        messageRead->_Data.Descriptor.SimpleFtlPayload.Lba = lba;
        messageRead->_Data.Descriptor.SimpleFtlPayload.SectorCount = sectorCount;
        clientCustomProtocolCmd->Push(messageRead);

        //Wait for read command response
        while (!clientCustomProtocolCmd->HasResponse());
        auto responseMessageRead = clientCustomProtocolCmd->PopResponse();

        //Get message read buffer to verify with the write buffer
        auto pReadData = clientCustomProtocolCmd->GetMessage(responseMessageRead->Id())->_Payload;
        auto result = std::memcmp(responseMessageWrite->_Payload, pReadData,
            sectorCount * responseGetDeviceInfo->_Data.Descriptor.DeviceInfoPayload.BytesPerSector);
        ASSERT_EQ(0, result);
    }

    //TODO: implement way to get server name
    constexpr char* messagingName = "SsdSimMainMessageServer";	//TODO: define a way to get name
    auto client = std::make_shared<MessageClient<SimFrameworkCommand>>(messagingName);
    ASSERT_NE(nullptr, client);

    auto message = AllocateMessage<SimFrameworkCommand>(client, 0, false);
    ASSERT_NE(message, nullptr);
    message->_Data.Code = SimFrameworkCommand::Code::Exit;
    client->Push(message);

    //Give the Framework a chance to stop completely before next test
    // This is a work around until multiple servers can be created without collision
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
}

TEST(SimpleFtl, Basic_DescendingWriteReadVerifyAll)
{
    Framework framework;
    ASSERT_NO_THROW(framework.Init("Hardwareconfig/hardwaremin.json"));

    auto fwFuture = std::async(std::launch::async, &(Framework::operator()), &framework);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    //TODO: implement way to get server name
    constexpr char* customMessagingName = "SsdSimCustomProtocolServer";
    auto clientCustomProtocolCmd = std::make_shared<MessageClient<CustomProtocolCommand>>(customMessagingName);
    ASSERT_NE(nullptr, clientCustomProtocolCmd);

    //Load dll SimpleFtl by sending command DownloadAndExecute
    constexpr char* simpleFtlDll = "SimpleFtl.dll";
    auto messageDownloadAndExecute = AllocateMessage<CustomProtocolCommand>(clientCustomProtocolCmd, 0, false);
    ASSERT_NE(messageDownloadAndExecute, nullptr);
    messageDownloadAndExecute->_Data.Command = CustomProtocolCommand::Code::DownloadAndExecute;
    memcpy(messageDownloadAndExecute->_Data.Descriptor.DownloadAndExecute.CodeName, simpleFtlDll,
        sizeof(messageDownloadAndExecute->_Data.Descriptor.DownloadAndExecute.CodeName));
    clientCustomProtocolCmd->Push(messageDownloadAndExecute);

    //Get device info to get lba count
    auto messageGetDeviceInfo = AllocateMessage<CustomProtocolCommand>(clientCustomProtocolCmd, 0, true);
    ASSERT_NE(messageGetDeviceInfo, nullptr);
    messageGetDeviceInfo->_Data.Command = CustomProtocolCommand::Code::GetDeviceInfo;
    clientCustomProtocolCmd->Push(messageGetDeviceInfo);

    while (!clientCustomProtocolCmd->HasResponse());
    auto responseGetDeviceInfo = clientCustomProtocolCmd->PopResponse();
    U32 lbaCount = responseGetDeviceInfo->_Data.Descriptor.DeviceInfoPayload.LbaCount;

    constexpr U32 maxSectorPerTransfer = 256;
    U32 payloadSize = maxSectorPerTransfer * responseGetDeviceInfo->_Data.Descriptor.DeviceInfoPayload.BytesPerSector;
    auto messageWrite = AllocateMessage<CustomProtocolCommand>(clientCustomProtocolCmd, payloadSize, true);
    ASSERT_NE(messageWrite, nullptr);
    ASSERT_NE(messageWrite->_Payload, nullptr);
    auto messageRead = AllocateMessage<CustomProtocolCommand>(clientCustomProtocolCmd, payloadSize, true);
    ASSERT_NE(messageRead, nullptr);

    //Write and read to verify all written data in ascending order
    U32 sectorCount;
    for (U32 lba(lbaCount); lba > 0; )
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
        for (U32 i(0); i < payloadSize; ++i)
        {
            ((U8*)messageWrite->_Payload)[i] = i % 255;
        }
        messageWrite->_Data.Command = CustomProtocolCommand::Code::Write;
        messageWrite->_Data.Descriptor.SimpleFtlPayload.Lba = lba;
        messageWrite->_Data.Descriptor.SimpleFtlPayload.SectorCount = sectorCount;
        clientCustomProtocolCmd->Push(messageWrite);

        //Wait for write command response
        while (!clientCustomProtocolCmd->HasResponse());
        auto responseMessageWrite = clientCustomProtocolCmd->PopResponse();

        messageRead->_Data.Command = CustomProtocolCommand::Code::Read;
        messageRead->_Data.Descriptor.SimpleFtlPayload.Lba = lba;
        messageRead->_Data.Descriptor.SimpleFtlPayload.SectorCount = sectorCount;
        clientCustomProtocolCmd->Push(messageRead);

        //Wait for read command response
        while (!clientCustomProtocolCmd->HasResponse());
        auto responseMessageRead = clientCustomProtocolCmd->PopResponse();

        //Get message read buffer to verify with the write buffer
        auto pReadData = clientCustomProtocolCmd->GetMessage(responseMessageRead->Id())->_Payload;
        auto result = std::memcmp(responseMessageWrite->_Payload, pReadData,
            sectorCount * responseGetDeviceInfo->_Data.Descriptor.DeviceInfoPayload.BytesPerSector);
        ASSERT_EQ(0, result);
    }

    //TODO: implement way to get server name
    constexpr char* messagingName = "SsdSimMainMessageServer";	//TODO: define a way to get name
    auto client = std::make_shared<MessageClient<SimFrameworkCommand>>(messagingName);
    ASSERT_NE(nullptr, client);

    auto message = AllocateMessage<SimFrameworkCommand>(client, 0, false);
    ASSERT_NE(message, nullptr);
    message->_Data.Code = SimFrameworkCommand::Code::Exit;
    client->Push(message);

    //Give the Framework a chance to stop completely before next test
    // This is a work around until multiple servers can be created without collision
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
}

TEST(SimpleFtl, Basic_WriteReadBenchmark)
{
    //TODO: implement way to get server name
    constexpr char* customMessagingName = "SsdSimCustomProtocolServer";

    Framework framework;
    ASSERT_NO_THROW(framework.Init("Hardwareconfig/hardwarespec.json"));

    auto fwFuture = std::async(std::launch::async, &(Framework::operator()), &framework);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    auto customClient = std::make_shared<MessageClient<CustomProtocolCommand>>(customMessagingName);
    ASSERT_NE(nullptr, customClient);

    //Load dll SimpleFtl by sending command DownloadAndExecute
    constexpr char* simpleFtlDll = "SimpleFtl.dll";
    auto messageDownloadAndExecute = AllocateMessage<CustomProtocolCommand>(customClient, 0, false);
    ASSERT_NE(messageDownloadAndExecute, nullptr);
    messageDownloadAndExecute->_Data.Command = CustomProtocolCommand::Code::DownloadAndExecute;
    memcpy(messageDownloadAndExecute->_Data.Descriptor.DownloadAndExecute.CodeName, simpleFtlDll,
        sizeof(messageDownloadAndExecute->_Data.Descriptor.DownloadAndExecute.CodeName));
    customClient->Push(messageDownloadAndExecute);

    using namespace std::chrono;

    constexpr U32 payloadSize = 128 * 1024;
    constexpr U32 commandCount = 10;
    constexpr U32 lba = 0;
    constexpr U32 sectorCount = 256;
    Message<CustomProtocolCommand>* messages[commandCount];
    for (auto i = 0; i < commandCount; ++i)
    {
        messages[i] = AllocateMessage<CustomProtocolCommand>(customClient, payloadSize, true);
        ASSERT_NE(messages[i], nullptr);
    }

    unsigned long totalBytesWrittenInSeconds = 0;
    double writeRate = 0;
    duration<double> dataCmdTotalTime = duration<double>::zero();
    std::map<U32, high_resolution_clock::time_point> t0s;
    for (auto i = 0; i < commandCount; ++i)
    {
        messages[i]->_Data.Command = CustomProtocolCommand::Code::Write;
        messages[i]->_Data.Descriptor.SimpleFtlPayload.Lba = lba;
        messages[i]->_Data.Descriptor.SimpleFtlPayload.SectorCount = sectorCount;
        t0s.insert(std::make_pair(messages[i]->Id(), high_resolution_clock::now()));
        memset(messages[i]->_Payload, 0xaa, messages[i]->_PayloadSize);
        customClient->Push(messages[i]);
    }

    U32 responseReceivedCount = 0;
    while (responseReceivedCount < commandCount)
    {
        if (customClient->HasResponse())
        {
            messages[responseReceivedCount] = customClient->PopResponse();

            auto deltaT = duration_cast<duration<double>>(high_resolution_clock::now() - t0s.find(messages[responseReceivedCount]->Id())->second);
            dataCmdTotalTime += deltaT;
            totalBytesWrittenInSeconds += messages[responseReceivedCount]->_PayloadSize;

            responseReceivedCount++;
        }
    }
    writeRate = (double)(totalBytesWrittenInSeconds / 1024 / 1024) / dataCmdTotalTime.count();

    //--Read in benchmark
    unsigned long totalBytesReadInSeconds = 0;
    double readRate = 0;
    dataCmdTotalTime = duration<double>::zero();
    t0s.clear();
    for (auto i = 0; i < commandCount; ++i)
    {
        messages[i]->_Data.Command = CustomProtocolCommand::Code::Read;
        messages[i]->_Data.Descriptor.SimpleFtlPayload.Lba = lba;
        messages[i]->_Data.Descriptor.SimpleFtlPayload.SectorCount = sectorCount;
        t0s.insert(std::make_pair(messages[i]->Id(), high_resolution_clock::now()));
        customClient->Push(messages[i]);
    }

    responseReceivedCount = 0;
    while (responseReceivedCount < commandCount)
    {
        if (customClient->HasResponse())
        {
            messages[responseReceivedCount] = customClient->PopResponse();

            auto deltaT = duration_cast<duration<double>>(high_resolution_clock::now() - t0s.find(messages[responseReceivedCount]->Id())->second);
            dataCmdTotalTime += deltaT;
            totalBytesReadInSeconds += messages[responseReceivedCount]->_PayloadSize;

            responseReceivedCount++;
        }
    }
    readRate = (double)(totalBytesReadInSeconds / 1024 / 1024) / dataCmdTotalTime.count();

    GOUT("Write/Read benchmark");
    GOUT("   Write rate: " << writeRate << " MB/s");
    GOUT("   Read rate: " << readRate << " MB/s");

    //--Deallocate
    for (auto i = 0; i < commandCount; ++i)
    {
        customClient->DeallocateMessage(messages[i]);
    }

    constexpr char* messagingName = "SsdSimMainMessageServer";	//TODO: define a way to get name

    auto client = std::make_shared<MessageClient<SimFrameworkCommand>>(messagingName);
    ASSERT_NE(nullptr, client);

    auto message = AllocateMessage<SimFrameworkCommand>(client, 0, false);
    ASSERT_NE(message, nullptr);
    message->_Data.Code = SimFrameworkCommand::Code::Exit;
    client->Push(message);

    //Give the Framework a chance to stop completely before next test
    // This is a work around until multiple servers can be created without collision
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
}