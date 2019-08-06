#pragma once
#ifndef __MessageServer_h__
#define __MeesageServer_h__

#include <memory>
#include <iostream>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/deque.hpp>

#include "MessageBaseService.hpp"

using namespace boost::interprocess;

template<typename TData>
class MessageServer : public MessageBaseService<TData>
{
public:
    MessageServer(const char* serverName)
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

    MessageServer(const char* serverName, const U32 &size, bool force = true)
    {
        if (force)
        {
            shared_memory_object::remove(serverName);
        }

        try
        {
            MessageBaseService<TData>::_ManagedShm = std::unique_ptr<managed_shared_memory>(new managed_shared_memory(open_or_create, serverName, size));
        }
        catch (...)
        {
            throw "Shared memory with the name " + std::string(serverName) + " already exists";
        }

        SynchronizedQueue<MessageId>::allocator_type alloc(MessageBaseService<TData>::_ManagedShm->get_segment_manager());
        SynchronizedQueue<MessageId>::allocator_type alloc1(MessageBaseService<TData>::_ManagedShm->get_segment_manager());

        MessageBaseService<TData>::_Counter = MessageBaseService<TData>::_ManagedShm->construct<MessageId>(COUNTER)(0);
        MessageBaseService<TData>::_Queue = MessageBaseService<TData>::_ManagedShm->construct<MessageQueue>(QUEUE)(alloc);
        MessageBaseService<TData>::_ResponseQueue = MessageBaseService<TData>::_ManagedShm->construct<MessageQueue>(RESPONSE_QUEUE)(alloc1);
    }

    bool HasMessage()
    {
        return !MessageBaseService<TData>::_Queue->empty();
    }

    Message<TData>* Pop()
    {
        return MessageBaseService<TData>::DoPop(MessageBaseService<TData>::_Queue);
    }

    void PushResponse(Message<TData>* message)
    {
        if (!message->ExpectsResponse())
        {
            throw "This message doesn't need respond";
        }

        message->_ResponseTime = std::chrono::high_resolution_clock::now();
        MessageBaseService<TData>::DoPush(MessageBaseService<TData>::_ResponseQueue, message);
    }

    void PushResponse(const MessageId &id)
    {
        Message<TData>* message = GetMessage(id);
        PushResponse(message);
    }

    void DeallocateMessage(Message<TData>* message)
    {
        if (message->ExpectsResponse())
        {
            throw "This message needs respond";
        }

        MessageBaseService<TData>::DoDeallocateMessage(message);
    }

    void DeallocateMessage(const MessageId &id)
    {
        Message<TData>* message = GetMessage(id);
        DeallocateMessage(message);
    }

    Message<TData>* GetMessage(const MessageId &id)
    {
        return MessageBaseService<TData>::GetMessage(id);
    }
};

#endif