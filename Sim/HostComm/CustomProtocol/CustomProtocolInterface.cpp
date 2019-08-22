#include "CustomProtocolInterface.h"

CustomProtocolInterface::CustomProtocolInterface(const char *protocolIpcName, BufferHal *bufferHal)
{
    _MessageServer = std::make_unique<MessageServer<CustomProtocolCommand>>(protocolIpcName);
    _BufferHal = bufferHal;
}

bool CustomProtocolInterface::HasCommand()
{
    return _MessageServer->HasMessage();
}

CustomProtocolCommand* CustomProtocolInterface::GetCommand()
{
    Message<CustomProtocolCommand>* msg = _MessageServer->Pop();
    if (msg)
    {
        msg->Data.CommandId = msg->Id();
        return &msg->Data;
    }

    return nullptr;
}

void CustomProtocolInterface::SubmitResponse(CustomProtocolCommand *command)
{
    Message<CustomProtocolCommand> *message = _MessageServer->GetMessage(command->CommandId);
    if (message->ExpectsResponse())
    {
        _MessageServer->PushResponse(message);
    }
    else
    {
        _MessageServer->DeallocateMessage(message);
    }
}

void CustomProtocolInterface::TransferIn(CustomProtocolCommand *command, const Buffer &inBuffer, const U32 &sectorIndex)
{
    U8 *buffer = GetBuffer(command, sectorIndex);
    _BufferHal->Memcpy(inBuffer, buffer);
}

void CustomProtocolInterface::TransferOut(CustomProtocolCommand *command, const Buffer &outBuffer, const U32 &sectorIndex)
{
    U8 *buffer = GetBuffer(command, sectorIndex);
    _BufferHal->Memcpy(buffer, outBuffer);
}

U8* CustomProtocolInterface::GetBuffer(CustomProtocolCommand *command, const U32 &sectorIndex)
{
    Message<CustomProtocolCommand>* msg = _MessageServer->GetMessage(command->CommandId);
    if (msg)
    {
        U32 bufferIndex = _BufferHal->ToByteIndexInTransfer(BufferType::User, sectorIndex);
        assert(msg->PayloadSize > bufferIndex);
        return ((U8*)msg->Payload) + bufferIndex;
    }

    return nullptr;
}
