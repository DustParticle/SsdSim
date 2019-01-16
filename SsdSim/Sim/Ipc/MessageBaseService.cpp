#include "MessageBaseService.h"
#include "Constant.h"

Message* MessageBaseService::DoAllocateMessage(Message::Type type, const U32 &payloadSize, const bool &expectsResponse)
{
    void* payload = nullptr;
    boost::interprocess::managed_shared_memory::handle_t handle = 0;
    if (payloadSize)
    {
        payload = _ManagedShm->allocate(payloadSize, std::nothrow);
        handle = _ManagedShm->get_handle_from_address(payload);
    }

    std::string messageName = GetMessageName(*_Counter);
    Message* message = _ManagedShm->construct<Message>(messageName.c_str())();
    message->_Id = *_Counter;
    message->_Type = type;
    message->_PayloadSize = payloadSize;
    message->_PayloadHandle = handle;
    message->_Payload = payload;
    message->_ExpectsResponse = expectsResponse;
    (*_Counter)++;

    return message;
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
    
    std::stringstream ss;
    ss << "Cannot find message with id " << id;
    throw ss.str();
}

std::string MessageBaseService::GetMessageName(const U32 &id)
{
    std::ostringstream stringStream;
    stringStream << MESSAGE;
    stringStream << id;
    return stringStream.str();
}