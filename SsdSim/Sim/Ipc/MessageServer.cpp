#include "MessageServer.h"
#include "Constant.h"

MessageServer::MessageServer(const char* serverName, const U32 &size)
{
    shared_memory_object::remove(serverName);
    _ManagedShm = std::unique_ptr<managed_shared_memory>(new managed_shared_memory(open_or_create, serverName, size));
    _Lock = _ManagedShm->construct<bool>(LOCK)(false);
    _Counter = _ManagedShm->construct<U32>(COUNTER)(0);
    _Queue = _ManagedShm->construct<deque<U32>>(QUEUE)();
    _ResponseQueueLock = _ManagedShm->construct<bool>(RESPONSE_QUEUE_LOCK)(false);
    _ResponseQueue = _ManagedShm->construct<deque<U32>>(RESPONSE_QUEUE)();
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
    return DoPop(_Lock, _Queue);
}

bool MessageServer::PushResponse(Message* message)
{
    if (((U16)message->_Type & RESPONSE_TYPE_FLAG) == 0)
    {
        return false;
    }

    DoPush(_ResponseQueueLock, _ResponseQueue, message);
    return true;
}

bool MessageServer::DeallocateMessage(Message* message)
{
    if (((U16)message->_Type & RESPONSE_TYPE_FLAG))
    {
        return false;
    }

    MessageBaseService::DoDeallocateMessage(message);
    return true;
}