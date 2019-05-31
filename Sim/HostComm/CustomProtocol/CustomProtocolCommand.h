#ifndef __CustomProtocolCommand_h__
#define __CustomProtocolCommand_h__

#include "HostComm/BasicTypes.h"

class CustomProtocolInterface;

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
    U32 BytesPerSector;
	U8	SectorsPerPage;
};

union CustomProtocolCommandDescriptor
{
    DownloadAndExecutePayload DownloadAndExecute;
    BenchmarkPayload BenchmarkPayload;
    SimpleFtlPayload SimpleFtlPayload;
    DeviceInfoPayload DeviceInfoPayload;
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
        Nop
    };

	enum class Status
	{
		Success,
		ReadError,
		WriteError,
	};

public:
    Code Command;
	Status CommandStatus;
    CustomProtocolCommandDescriptor Descriptor;

private:
    CommandId CommandId;
    friend class CustomProtocolInterface;
};

#endif