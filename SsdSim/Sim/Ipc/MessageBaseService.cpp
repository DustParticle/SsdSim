#include "MessageBaseService.h"
#include "Constant.h"

Message* MessageBaseService::DoPop(bool* lock, deque<U32>* queue)
{
    if (*lock)
    {
        return nullptr;
    }

    if (queue->empty())
    {
        return nullptr;
    }

    U32 id = queue->front();
    queue->pop_front();

    std::string messageName = GetMessageName(id);
    auto message = _ManagedShm->find<Message>(messageName.c_str());
    if (message.first)
    {
        if (message.first->_PayloadSize)
        {
            message.first->_Payload = _ManagedShm->get_address_from_handle(message.first->_PayloadHandle);
        }
        return message.first;
    }

    return nullptr;
}

void MessageBaseService::DoDeallocateMessage(Message* message)
{
    if (message->_PayloadSize)
    {
        _ManagedShm->deallocate(message->_Payload);
    }

    std::string messageName = GetMessageName(message->_Id);
    _ManagedShm->destroy<Message>(messageName.c_str());
}

void MessageBaseService::DoPush(bool* lock, deque<U32>* queue, Message* message)
{
    *lock = true;
    queue->push_back(message->_Id);
    *lock = false;
}

std::string MessageBaseService::GetMessageName(const U32 &id)
{
    std::ostringstream stringStream;
    stringStream << MESSAGE;
    stringStream << id;
    return stringStream.str();
}