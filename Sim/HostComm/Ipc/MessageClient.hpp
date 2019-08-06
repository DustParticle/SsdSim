#pragma once
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
        MessageBaseService<TData>::_Counter = MessageBaseService<TData>::_ManagedShm->find<MessageId>(COUNTER).first;
        MessageBaseService<TData>::_Queue = MessageBaseService<TData>::_ManagedShm->find<MessageQueue>(QUEUE).first;
        MessageBaseService<TData>::_ResponseQueue = MessageBaseService<TData>::_ManagedShm->find<MessageQueue>(RESPONSE_QUEUE).first;
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
        message->_SubmitTime = std::chrono::high_resolution_clock::now();
        MessageBaseService<TData>::DoPush(MessageBaseService<TData>::_Queue, message);
    }

    bool HasResponse()
    {
        return !MessageBaseService<TData>::_ResponseQueue->empty();
    }

    Message<TData>* PopResponse()
    {
        return MessageBaseService<TData>::DoPop(MessageBaseService<TData>::_ResponseQueue);
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