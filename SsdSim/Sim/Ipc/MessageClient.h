#ifndef __MessageClient_h__
#define __MessageClient_h__

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/deque.hpp>

#include "Common/BasicTypes.h"
#include "MessageBaseService.h"
#include "Message.h"

using namespace boost::interprocess;

class MessageClient : MessageBaseService
{
public:
    MessageClient(const char* serverName);
    ~MessageClient();

    Message* AllocateMessage(Message::Type type, const U32 &payloadSize = 0);
    void Push(Message* message);
    bool HasResponse();
    Message* PopResponse();
    void DeallocateMessage(Message* message);
};

#endif