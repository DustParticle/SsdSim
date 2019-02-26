#include "MessageClient.h"
#include "Constant.h"

MessageClient::MessageClient(const char* serverName)
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

MessageClient::~MessageClient()
{
}

Message* MessageClient::AllocateMessage(Message::Type type, const U32 &payloadSize, const bool &expectsResponse)
{
    return DoAllocateMessage(type, payloadSize, expectsResponse);
}

void MessageClient::Push(Message* message)
{
    DoPush(_Lock, _Queue, message);
}

bool MessageClient::HasResponse()
{
    if (*_ResponseQueueLock)
    {
        return false;
    }

    return !_ResponseQueue->empty();
}

Message* MessageClient::PopResponse()
{
    return DoPop(_ResponseQueueLock, _ResponseQueue);
}

void MessageClient::DeallocateMessage(Message* message)
{
    MessageBaseService::DoDeallocateMessage(message);
}

Message* MessageClient::GetMessage(const MessageId &id)
{
    return MessageBaseService::GetMessage(id);
}