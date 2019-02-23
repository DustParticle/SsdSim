#ifndef __CustomProtocolCommand_h__
#define __CustomProtocolCommand_h__

#include "Common/BasicTypes.h"

enum CommandCode
{
    DownloadAndExecute
};

struct DownloadAndExecutePayload
{
    U8 CodeName[20];
    U32 CodeSize;
    U8 CodeBinary[1];
};

union CustomProtocolCommandPayload
{
    DownloadAndExecutePayload DownloadAndExecute;
};

struct CustomProtocolCommand
{
public:
    CommandCode Command;
    CustomProtocolCommandPayload Payload;
};

#endif