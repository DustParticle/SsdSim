#ifndef __CustomProtocolInterface_h__
#define __CustomProtocolInterface_h__

#include "Common/BasicTypes.h"
#include "CustomProtocolCommand.h"

class CustomProtocolInterface
{
public:
    static bool HasCommand();
    static CustomProtocolCommand* GetCommand();
    static void SubmitResponse(CustomProtocolCommand *command);
};

#endif