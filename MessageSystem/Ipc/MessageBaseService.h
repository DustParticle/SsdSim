#ifndef __MessageBaseService_h__
#define __MessageBaseService_h__

#include <memory>
#include <iostream>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/deque.hpp>

#include "Message.h"

using namespace boost::interprocess;

class MessageBaseService
{
protected: 
    std::unique_ptr<managed_shared_memory> _ManagedShm;
    bool *_Lock;
    U32 *_Counter;
    deque<U32> *_Queue;
    bool *_ResponseQueueLock;
    deque<U32> *_ResponseQueue;

protected:
    Message * DoAllocateMessage(Message::Type type, const U32 &payloadSize, const bool &expectsResponse);
    void DoDeallocateMessage(Message* message);

    Message * DoPop(bool* lock, deque<U32>* queue);
    void DoPush(bool* lock, deque<U32>* queue, Message* message);
    std::string GetMessageName(const U32 &id);
};

#endif