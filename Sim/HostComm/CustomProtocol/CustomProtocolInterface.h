#ifndef __CustomProtocolInterface_h__
#define __CustomProtocolInterface_h__

#include "HostComm/BasicTypes.h"
#include "CustomProtocolCommand.h"
#include "HostComm/Ipc/MessageServer.hpp"

class CustomProtocolInterface
{
public:
    CustomProtocolInterface(const char *protocolIpcName = nullptr);
    bool HasCommand();
    CustomProtocolCommand* GetCommand();
    U8* GetBuffer(CustomProtocolCommand *command, U32 &bufferSizeInBytes);
    void SubmitResponse(CustomProtocolCommand *command);

private:
    std::unique_ptr<MessageServer<CustomProtocolCommand>> _MessageServer;
};

#endif