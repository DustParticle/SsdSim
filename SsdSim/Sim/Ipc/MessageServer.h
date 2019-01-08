#ifndef __MessageServer_h__
#define __MeesageServer_h__

#include <memory>
#include <iostream>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/deque.hpp>

#include "Message.h"

using namespace boost::interprocess;

class MessageServer
{
private: 
    std::unique_ptr<managed_shared_memory> _ManagedShm;
    bool *_Lock;
    U32 *_Counter;
    deque<U32> *_Queue;

public:
    MessageServer(const char* serverName, const U32 &size);
    bool HasMessage();
    Message* Pop();
    void DeallocateMessage(Message* message);
};

#endif