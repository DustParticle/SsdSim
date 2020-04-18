#include "CustomProtocolHal.h"

CustomProtocolHal::CustomProtocolHal()
{
    _TransferCommandQueue = std::unique_ptr<boost::lockfree::spsc_queue<TransferCommandDesc>>(new boost::lockfree::spsc_queue<TransferCommandDesc>{ 1024 });
}

void CustomProtocolHal::Init(const char *protocolIpcName, BufferHal *bufferHal)
{
    _MessageServer = std::make_unique<MessageServer<CustomProtocolCommand>>(protocolIpcName);
    _BufferHal = bufferHal;
}

bool CustomProtocolHal::HasCommand()
{
    return _MessageServer->HasMessage();
}

CustomProtocolCommand* CustomProtocolHal::GetCommand()
{
    Message<CustomProtocolCommand>* msg = _MessageServer->Pop();
    if (msg)
    {
        msg->Data.CommandId = msg->Id();
        return &msg->Data;
    }

    return nullptr;
}

void CustomProtocolHal::SubmitResponse(CustomProtocolCommand *command)
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

void CustomProtocolHal::QueueCommand(const TransferCommandDesc& command)
{
    _TransferCommandQueue->push(command);
}

U8* CustomProtocolHal::GetBuffer(CustomProtocolCommand *command, const tSectorOffset& offset)
{
    Message<CustomProtocolCommand>* msg = _MessageServer->GetMessage(command->CommandId);
    if (msg)
    {
        U32 bufferIndex = _BufferHal->ToByteIndexInTransfer(BufferType::User, offset._);
        assert(msg->PayloadSize > bufferIndex);
        return ((U8*)msg->Payload) + bufferIndex;
    }

    return nullptr;
}

void CustomProtocolHal::Run()
{
    while (_TransferCommandQueue->empty() == false)
    {
        ProcessTransferCommand();
    }
}

void CustomProtocolHal::ProcessTransferCommand()
{
    TransferCommandDesc& command = _TransferCommandQueue->front();
    U8 *buffer = GetBuffer(command.Command, command.CommandOffset);
    if (command.Direction == TransferCommandDesc::Direction::In)
    {
        _BufferHal->Memcpy(command.Buffer, command.BufferOffset, buffer, command.SectorCount);
    }
    else
    {
        _BufferHal->Memcpy(buffer, command.Buffer, command.BufferOffset, command.SectorCount);
    }

    assert(command.Listener != nullptr);
    command.Listener->HandleCommandCompleted(command);
    _TransferCommandQueue->pop();
}
