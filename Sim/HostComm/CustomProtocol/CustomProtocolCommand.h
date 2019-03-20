#ifndef __CustomProtocolCommand_h__
#define __CustomProtocolCommand_h__

#include "HostComm/BasicTypes.h"

class CustomProtocolInterface;

struct DownloadAndExecutePayload
{
    U8 _CodeName[20];
};

union BenchmarkPayload
{
    struct
    {
        long long _Duration;
        U32 _NopCount;
    } _Response;
};

struct SimpleFtlPayload
{
    U32 _Lba;
    U32 _SectorCount;
};

struct DeviceInfoPayload
{
    U32 _LbaCount;
    U32 _BytesPerSector;
};

union CustomProtocolCommandPayload
{
    DownloadAndExecutePayload _DownloadAndExecute;
    BenchmarkPayload _BenchmarkPayload;
    SimpleFtlPayload _SimpleFtlPayload;
    DeviceInfoPayload _DeviceInfoPayload;
};

typedef U32 CommandId;

struct CustomProtocolCommand
{
    enum Code
    {
        DownloadAndExecute,
        BenchmarkStart,
        BenchmarkEnd,
        Write,
        Read,
        GetDeviceInfo,
        Nop
    };

public:
    Code _Command;
    CustomProtocolCommandPayload _Payload;

private:
    CommandId CommandId;
    friend class CustomProtocolInterface;
};

#endif