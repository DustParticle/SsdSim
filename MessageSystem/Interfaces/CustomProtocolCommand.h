#ifndef __CustomProtocolCommand_h__
#define __CustomProtocolCommand_h__

#include "Common/BasicTypes.h"

// TODO: add macro to check for sim only
#include "Ipc/Message.hpp"
class CustomProtocolInterface;

struct DownloadAndExecutePayload
{
    U8 CodeName[20];
    U32 CodeSize;
    U8 CodeBinary[1];
};

union BenchmarkPayload
{
    struct
    {
        long long Duration;
        U32 NopCount;
    } Response;
};

union CustomProtocolCommandPayload
{
    DownloadAndExecutePayload DownloadAndExecute;
    BenchmarkPayload BenchmarkPayload;
};

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
    Code Command;
    CustomProtocolCommandPayload Payload;

private:
    // TODO: add macro to check for sim only
    MessageId MessageId;
    friend class CustomProtocolInterface;
};

#endif