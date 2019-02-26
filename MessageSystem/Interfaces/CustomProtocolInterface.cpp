#include "CustomProtocolInterface.h"

#include "MessageServer.h"
#include "ServerNames.h"

MessageServer _MessageServer(PROTOCOL_IPC_NAME);

bool CustomProtocolInterface::HasCommand()
{
    return _MessageServer.HasMessage();
}

CustomProtocolCommand* CustomProtocolInterface::GetCommand()
{
    Message* msg = _MessageServer.Pop();
    if (msg)
    {
        if (msg->_Type == Message::Type::CUSTOM_PROTOCOL_COMMAND)
        {
            CustomProtocolCommand* command = (CustomProtocolCommand*)msg->_Payload;
            command->MessageId = msg->Id();
            return command;
        }
        else
        {
            if (msg->ExpectsResponse())
            {
                _MessageServer.PushResponse(msg);
            }
            else
            {
                _MessageServer.DeallocateMessage(msg);
            }
        }
    }

    return nullptr;
}

void CustomProtocolInterface::SubmitResponse(CustomProtocolCommand *command)
{
    Message *message = _MessageServer.GetMessage(command->MessageId);
    if (message->ExpectsResponse())
    {
        _MessageServer.PushResponse(message);
    }
    else
    {
        _MessageServer.DeallocateMessage(message);
    }
}