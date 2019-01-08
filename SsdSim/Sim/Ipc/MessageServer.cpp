#include "MessageServer.h"
#include "Constant.h"

MessageServer::MessageServer(const char* serverName, const U32 &size)
{
    shared_memory_object::remove(serverName);
    _ManagedShm = std::unique_ptr<managed_shared_memory>(new managed_shared_memory(open_or_create, serverName, size));
    _Lock = _ManagedShm->construct<bool>(LOCK)(false);
    _Counter = _ManagedShm->construct<U32>(COUNTER)(0);
    _Queue = _ManagedShm->construct<deque<U32>>(QUEUE)();
}

bool MessageServer::HasMessage()
{
    if (*_Lock)
    {
        return false;
    }

    return !_Queue->empty();
}

Message* MessageServer::Pop()
{
    if (*_Lock)
    {
        return nullptr;
    }

    if (_Queue->empty())
    {
        return nullptr;
    }

    int id = _Queue->front();
    _Queue->pop_front();

    std::ostringstream stringStream;
    stringStream << MESSAGE;
    stringStream << id;

    auto message = _ManagedShm->find<Message>(stringStream.str().c_str());
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

void MessageServer::DeallocateMessage(Message* message)
{
    if (message->_PayloadSize)
    {
        _ManagedShm->deallocate(message->_Payload);
    }

    std::ostringstream stringStream;
    stringStream << MESSAGE;
    stringStream << message->_Id;
    _ManagedShm->destroy<Message>(stringStream.str().c_str());
}

