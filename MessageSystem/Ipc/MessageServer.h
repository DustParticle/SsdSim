#ifndef __MessageServer_h__
#define __MeesageServer_h__

#include <memory>
#include <iostream>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/deque.hpp>

#include "MessageBaseService.h"
#include "Message.h"

using namespace boost::interprocess;

class MessageServer : MessageBaseService
{
public:
    MessageServer(const char* serverName);
    MessageServer(const char* serverName, const U32 &size);
    bool HasMessage();
    Message* Pop();
    void PushResponse(Message* message);
    void PushResponse(const MessageId &id);
    void DeallocateMessage(Message* message);
    void DeallocateMessage(const MessageId &id);
    Message* GetMessage(const MessageId &id);
};

#endif