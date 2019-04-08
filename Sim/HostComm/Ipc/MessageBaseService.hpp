#pragma once
#ifndef __MessageBaseService_h__
#define __MessageBaseService_h__

#include <memory>
#include <iostream>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/deque.hpp>

#include "Message.hpp"
#include "Constant.h"
#include "SynchronizedQueue.hpp"

using namespace boost::interprocess;

typedef SynchronizedQueue<MessageId> MessageQueue;

template<typename TData>
class MessageBaseService
{
protected: 
    std::unique_ptr<managed_shared_memory> _ManagedShm;
    U32 *_Counter;
    MessageQueue *_Queue;
    MessageQueue *_ResponseQueue;

public:
    Message<TData>* GetMessage(const MessageId &id)
    {
        std::string messageName = GetMessageName(id);
        auto message = _ManagedShm->find<Message<TData>>(messageName.c_str());
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

protected:
    Message<TData>* DoAllocateMessage(const U32 &payloadSize, const bool &expectsResponse)
    {
        void* payload = nullptr;
        boost::interprocess::managed_shared_memory::handle_t handle = 0;
        if (payloadSize)
        {
            payload = _ManagedShm->allocate(payloadSize, std::nothrow);
            if (payload == nullptr)
            {
                throw "Not enough memory to allocate message";
            }
            handle = _ManagedShm->get_handle_from_address(payload);
        }

        std::string messageName = GetMessageName(*_Counter);
        Message<TData>* message = _ManagedShm->construct<Message<TData>>(messageName.c_str())();
        message->_Id = *_Counter;
        message->_PayloadSize = payloadSize;
        message->_PayloadHandle = handle;
        message->_Payload = payload;
        message->_ExpectsResponse = expectsResponse;
        (*_Counter)++;

        return message;
    }

    void DoDeallocateMessage(Message<TData>* message)
    {
        if (message->_PayloadSize)
        {
            _ManagedShm->deallocate(message->_Payload);
        }

        std::string messageName = GetMessageName(message->_Id);
        _ManagedShm->destroy<Message<TData>>(messageName.c_str());
    }

    void DoPush(MessageQueue* &queue, Message<TData>* message)
    {
        queue->push(message->_Id);
    }

    Message<TData>* DoPop(MessageQueue* &queue)
    {
        MessageId id;
        if (!queue->pop(id))
        {
            return nullptr;
        }

        return GetMessage(id);
    }

    std::string GetMessageName(const MessageId &id)
    {
        std::ostringstream stringStream;
        stringStream << MESSAGE;
        stringStream << id;
        return stringStream.str();
    }
};

#endif