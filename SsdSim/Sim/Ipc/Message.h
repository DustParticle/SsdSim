#ifndef __Message_h__
#define __Message_h__

#include <boost/interprocess/managed_shared_memory.hpp>

#include "Common/BasicTypes.h"

struct Message
{
    enum class Type
    {
        SIM_FRAMEWORK_COMMAND
    };

public:
	bool ExpectsResponse() { return _ExpectsResponse; }

public:
    Type _Type;
    void* _Payload;
    U32 _PayloadSize;

private:
    boost::interprocess::managed_shared_memory::handle_t _PayloadHandle;
    U32 _Id;
    bool _ExpectsResponse;

    friend class MessageBaseService;
};

#endif