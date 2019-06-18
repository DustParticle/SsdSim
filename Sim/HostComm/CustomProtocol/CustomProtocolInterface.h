#ifndef __CustomProtocolInterface_h__
#define __CustomProtocolInterface_h__

#include "HostComm/BasicTypes.h"
#include "CustomProtocolCommand.h"
#include "HostComm/Ipc/MessageServer.hpp"
#include "Buffer/Hal/BufferHal.h"

class CustomProtocolInterface
{
public:
    CustomProtocolInterface(const char *protocolIpcName = nullptr, BufferHal *bufferHal = nullptr);
    bool HasCommand();
    CustomProtocolCommand* GetCommand();
    void SubmitResponse(CustomProtocolCommand *command);

    void TransferIn(CustomProtocolCommand *command, const Buffer &inBuffer, const U32 &sectorIndex);
    void TransferOut(CustomProtocolCommand *command, const Buffer &outBuffer, const U32 &sectorIndex);

private:
    U8* GetBuffer(CustomProtocolCommand *command, U32 &bufferSizeInBytes, const U32 &sectorIndex);

private:
    std::unique_ptr<MessageServer<CustomProtocolCommand>> _MessageServer;
    BufferHal *_BufferHal;
};

#endif