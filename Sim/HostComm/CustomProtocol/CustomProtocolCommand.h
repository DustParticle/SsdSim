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

union CustomProtocolCommandPayload
{
    DownloadAndExecutePayload _DownloadAndExecute;
    BenchmarkPayload _BenchmarkPayload;
};

typedef U32 CommandId;

struct CustomProtocolCommand
{
    enum Code
    {
        DownloadAndExecute,
        BenchmarkStart,
        BenchmarkEnd,
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