#include "MessageServer.h"
#include "Constant.h"

MessageServer::MessageServer(const char* serverName)
{
    managed_shared_memory *sharedMemory;
    try
    {
        sharedMemory = new managed_shared_memory(open_only, serverName);
    }
    catch (...)
    {
        throw "Server does not exist";
    }

    _ManagedShm = std::unique_ptr<managed_shared_memory>(sharedMemory);
    _Lock = _ManagedShm->find<bool>(LOCK).first;
    _Counter = _ManagedShm->find<MessageId>(COUNTER).first;
    _Queue = _ManagedShm->find<deque<MessageId>>(QUEUE).first;
    _ResponseQueueLock = _ManagedShm->find<bool>(RESPONSE_QUEUE_LOCK).first;
    _ResponseQueue = _ManagedShm->find<deque<MessageId>>(RESPONSE_QUEUE).first;
}

MessageServer::MessageServer(const char* serverName, const U32 &size)
{
    shared_memory_object::remove(serverName);
    _ManagedShm = std::unique_ptr<managed_shared_memory>(new managed_shared_memory(open_or_create, serverName, size));
    _Lock = _ManagedShm->construct<bool>(LOCK)(false);
    _Counter = _ManagedShm->construct<MessageId>(COUNTER)(0);
    _Queue = _ManagedShm->construct<deque<MessageId>>(QUEUE)();
    _ResponseQueueLock = _ManagedShm->construct<bool>(RESPONSE_QUEUE_LOCK)(false);
    _ResponseQueue = _ManagedShm->construct<deque<MessageId>>(RESPONSE_QUEUE)();
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

void MessageServer::PushResponse(Message* message)
{
    if (!message->ExpectsResponse())
    {
        throw "This message doesn't need respond";
    }

    DoPush(_ResponseQueueLock, _ResponseQueue, message);
}

void MessageServer::PushResponse(const MessageId &id)
{
    Message* message = GetMessage(id);
    DeallocateMessage(message);
}

void MessageServer::DeallocateMessage(Message* message)
{
    if (message->ExpectsResponse())
    {
        throw "This message needs respond";
    }

    MessageBaseService::DoDeallocateMessage(message);
}

void MessageServer::DeallocateMessage(const MessageId &id)
{
    Message* message = GetMessage(id);
    DeallocateMessage(message);
}

Message* MessageServer::GetMessage(const MessageId &id)
{
    return MessageBaseService::GetMessage(id);
}