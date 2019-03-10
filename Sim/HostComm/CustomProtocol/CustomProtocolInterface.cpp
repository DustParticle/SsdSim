#include "CustomProtocolInterface.h"

#include "HostComm/Ipc/MessageServer.hpp"

MessageServer<CustomProtocolCommand> _MessageServer("SsdSimCustomProtocolServer");

bool CustomProtocolInterface::HasCommand()
{
    return _MessageServer.HasMessage();
}

CustomProtocolCommand* CustomProtocolInterface::GetCommand()
{
    Message<CustomProtocolCommand>* msg = _MessageServer.Pop();
    if (msg)
    {
        return &msg->_Data;
    }

    return nullptr;
}

void CustomProtocolInterface::SubmitResponse(CustomProtocolCommand *command)
{
    Message<CustomProtocolCommand> *message = _MessageServer.GetMessage(command->MessageId);
    if (message->ExpectsResponse())
    {
        _MessageServer.PushResponse(message);
    }
    else
    {
        _MessageServer.DeallocateMessage(message);
    }
}