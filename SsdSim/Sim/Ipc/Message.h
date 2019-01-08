#ifndef __Message_h__
#define __Message_h__

#include <boost/interprocess/managed_shared_memory.hpp>

#include "Common/BasicTypes.h"

class MessageServer;
class MessageClient;

struct Message
{
    enum class Type
    {
        NOP,
        Exit
    };


public:
    Type _Type;
    void* _Payload;
    U32 _PayloadSize;

private:
    boost::interprocess::managed_shared_memory::handle_t _PayloadHandle;
    U32 _Id;

    friend class MessageServer;
    friend class MessageClient;
};

#endif