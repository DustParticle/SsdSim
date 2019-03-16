#ifndef __CustomProtocolInterface_h__
#define __CustomProtocolInterface_h__

#include "HostComm/BasicTypes.h"
#include "CustomProtocolCommand.h"

class CustomProtocolInterface
{
public:
    bool HasCommand();
    CustomProtocolCommand* GetCommand();
    void SubmitResponse(CustomProtocolCommand *command);
};

#endif