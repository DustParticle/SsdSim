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
    _Counter = _ManagedShm->find<U32>(COUNTER).first;
    _Queue = _ManagedShm->find<deque<U32>>(QUEUE).first;
    _ResponseQueueLock = _ManagedShm->find<bool>(RESPONSE_QUEUE_LOCK).first;
    _ResponseQueue = _ManagedShm->find<deque<U32>>(RESPONSE_QUEUE).first;
}

MessageClient::~MessageClient()
{
}

Message* MessageClient::AllocateMessage(Message::Type type, const U32 &payloadSize)
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
    (*_Counter)++;

    return message;
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