#ifndef __Message_h__
#define __Message_h__

#include <boost/interprocess/managed_shared_memory.hpp>

#include "Common/BasicTypes.h"

constexpr U16 RESPONSE_TYPE_FLAG = 1 << 15;

struct Message
{
    enum class Type
    {
        SIM_FRAMEWORK_COMMAND = 1,
        SIM_FRAMEWORK_COMMAND_WITH_RESPONSE = 2 | RESPONSE_TYPE_FLAG,
    };


public:
    Type _Type;
    void* _Payload;
    U32 _PayloadSize;

private:
    boost::interprocess::managed_shared_memory::handle_t _PayloadHandle;
    U32 _Id;

    friend class MessageBaseService;
    friend class MessageServer;
    friend class MessageClient;
};

#endif