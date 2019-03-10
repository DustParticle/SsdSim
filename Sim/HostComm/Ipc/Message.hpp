#ifndef __Message_h__
#define __Message_h__

#include <boost/interprocess/managed_shared_memory.hpp>

#include "HostComm/BasicTypes.h"

typedef U32 MessageId;

template<typename TData>
struct Message
{
public:
	bool ExpectsResponse() { return _ExpectsResponse; }
    MessageId Id() { return _Id; }

public:
    TData _Data;
    void* _Payload;
    U32 _PayloadSize;

private:
    boost::interprocess::managed_shared_memory::handle_t _PayloadHandle;
    MessageId _Id;
    bool _ExpectsResponse;

    template<typename TData> friend class MessageBaseService;
};

#endif