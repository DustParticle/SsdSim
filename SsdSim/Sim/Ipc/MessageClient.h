#ifndef __MessageClient_h__
#define __MessageClient_h__

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/deque.hpp>

#include "Common/BasicTypes.h"
#include "Message.h"

using namespace boost::interprocess;

class MessageClient
{
private:
    std::unique_ptr<managed_shared_memory> _ManagedShm;
    bool *_Lock;
    U32 *_Counter;
    deque<U32> *_Queue;

public:
    MessageClient(const char* serverName);
    ~MessageClient();

    Message* AllocateMessage(Message::Type type, const U32 &payloadSize = 0);
    void Push(Message* message);
};

#endif