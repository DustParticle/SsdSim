#ifndef __CustomProtocolCommand_h__
#define __CustomProtocolCommand_h__

#include "HostComm/BasicTypes.h"
#include "Buffer/Types.h"

class CustomProtocolHal;

struct DownloadAndExecutePayload
{
    U8 CodeName[20];
};

union BenchmarkPayload
{
    struct
    {
        long long Duration;
        U32 NopCount;
    } Response;
};

struct SimpleFtlPayload
{
    U32 Lba;
    U32 SectorCount;
};

struct DeviceInfoPayload
{
    U32 TotalSector;
    SectorInfo SectorInfo;
	U8	SectorsPerPage;
};

struct SectorInfoPayload
{
    SectorInfo SectorInfo;
};

union CustomProtocolCommandDescriptor
{
    DownloadAndExecutePayload DownloadAndExecute;
    BenchmarkPayload BenchmarkPayload;
    SimpleFtlPayload SimpleFtlPayload;
    DeviceInfoPayload DeviceInfoPayload;
    SectorInfoPayload SectorInfoPayload;
};

typedef U32 CommandId;

struct CustomProtocolCommand
{
    enum class Code
    {
        DownloadAndExecute,
        BenchmarkStart,
        BenchmarkEnd,
        Write,
		LoopbackWrite,
        Read,
		LoopbackRead,
        GetDeviceInfo,
        SetSectorSize,
        Nop
    };

    enum class Status
    {
        Success,
        ReadError,
        WriteError,
        Failed,
	};

public:
    Code Command;
	Status CommandStatus;
    CustomProtocolCommandDescriptor Descriptor;

private:
    CommandId CommandId;
    friend class CustomProtocolHal;
};

#endif