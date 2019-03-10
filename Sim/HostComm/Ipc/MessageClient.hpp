#ifndef __MessageClient_h__
#define __MessageClient_h__

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/deque.hpp>

#include "HostComm/BasicTypes.h"
#include "MessageBaseService.hpp"

using namespace boost::interprocess;

template<typename TData>
class MessageClient : public MessageBaseService<TData>
{
public:
    MessageClient(const char* serverName)
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

        MessageBaseService<TData>::_ManagedShm = std::unique_ptr<managed_shared_memory>(sharedMemory);
        MessageBaseService<TData>::_Lock = MessageBaseService<TData>::_ManagedShm->find<bool>(LOCK).first;
        MessageBaseService<TData>::_Counter = MessageBaseService<TData>::_ManagedShm->find<MessageId>(COUNTER).first;
        MessageBaseService<TData>::_Queue = MessageBaseService<TData>::_ManagedShm->find<deque<MessageId>>(QUEUE).first;
        MessageBaseService<TData>::_ResponseQueueLock = MessageBaseService<TData>::_ManagedShm->find<bool>(RESPONSE_QUEUE_LOCK).first;
        MessageBaseService<TData>::_ResponseQueue = MessageBaseService<TData>::_ManagedShm->find<deque<MessageId>>(RESPONSE_QUEUE).first;
    }

    ~MessageClient()
    {
    }

    Message<TData>* AllocateMessage(const U32 &payloadSize = 0, const bool &expectsResponse = false)
    {
        return MessageBaseService<TData>::DoAllocateMessage(payloadSize, expectsResponse);
    }

    void Push(Message<TData>* message)
    {
        MessageBaseService<TData>::DoPush(MessageBaseService<TData>::_Lock, MessageBaseService<TData>::_Queue, message);
    }

    bool HasResponse()
    {
        if (*MessageBaseService<TData>::_ResponseQueueLock)
        {
            return false;
        }

        return !MessageBaseService<TData>::_ResponseQueue->empty();
    }

    Message<TData>* PopResponse()
    {
        return MessageBaseService<TData>::DoPop(MessageBaseService<TData>::_ResponseQueueLock, MessageBaseService<TData>::_ResponseQueue);
    }

    void DeallocateMessage(Message<TData>* message)
    {
        MessageBaseService<TData>::DoDeallocateMessage(message);
    }

    Message<TData>* GetMessage(const MessageId &id)
    {
        return MessageBaseService<TData>::GetMessage(id);
    }
};

#endif