#pragma once
#ifndef __Message_h__
#define __Message_h__

#include <chrono>

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
    TData Data;
    void* Payload;
    U32 PayloadSize;

public:
    std::chrono::duration<double> GetLatency()
    {
        return std::chrono::duration_cast<std::chrono::duration<double>>(_ResponseTime - _SubmitTime);
    }

private:
    boost::interprocess::managed_shared_memory::handle_t _PayloadHandle;
    MessageId _Id;
    bool _ExpectsResponse;

    std::chrono::high_resolution_clock::time_point _SubmitTime;
    std::chrono::high_resolution_clock::time_point _ResponseTime;

    template<typename TData> friend class MessageBaseService;
    template<typename TData> friend class MessageClient;
    template<typename TData> friend class MessageServer;
};

#endif