#include "CustomProtocolInterface.h"

CustomProtocolInterface::CustomProtocolInterface()
{
    _TransferCommandQueue = std::unique_ptr<boost::lockfree::spsc_queue<TransferCommandDesc>>(new boost::lockfree::spsc_queue<TransferCommandDesc>{ 1024 });
    _FinishedTransferCommandQueue = std::unique_ptr<boost::lockfree::spsc_queue<TransferCommandDesc>>(new boost::lockfree::spsc_queue<TransferCommandDesc>{ 1024 });
}

void CustomProtocolInterface::Init(const char *protocolIpcName, BufferHal *bufferHal)
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

void CustomProtocolInterface::QueueCommand(const TransferCommandDesc& command)
{
    _TransferCommandQueue->push(command);
}

bool CustomProtocolInterface::PopFinishedCommand(TransferCommandDesc& command)
{
    return (_FinishedTransferCommandQueue->pop(command));
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

void CustomProtocolInterface::Run()
{
    while (_TransferCommandQueue->empty() == false)
    {
        TransferCommandDesc& command = _TransferCommandQueue->front();
        U8 *buffer = GetBuffer(command.Command, command.SectorIndex);
        if (command.Direction == TransferCommandDesc::Direction::In)
        {
            _BufferHal->Memcpy(command.Buffer, buffer);
        }
        else
        {
            _BufferHal->Memcpy(buffer, command.Buffer);
        }

        _FinishedTransferCommandQueue->push(command);
        _TransferCommandQueue->pop();
    }
}
