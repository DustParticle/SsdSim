#include "InterfaceQueues.h"

#define CUSTOM_PROTOCOL_COMMAND_QUEUE_SIZE 5

InterfaceQueues::InterfaceQueues() : CustomProtocolCommandQueue(CUSTOM_PROTOCOL_COMMAND_QUEUE_SIZE)
{
}
