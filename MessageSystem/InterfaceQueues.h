#ifndef __InterfaceQueues_h__
#define __InterfaceQueues_h__

#include "boost/lockfree/spsc_queue.hpp"

#include "Common/BasicTypes.h"
#include "CustomProtocolCommand.h"

struct InterfaceQueues
{
public:
    boost::lockfree::spsc_queue<CustomProtocolCommand*> CustomProtocolCommandQueue;

public:
    InterfaceQueues();
};

#endif