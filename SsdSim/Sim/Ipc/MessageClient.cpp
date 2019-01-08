#include "MessageClient.h"
#include "Constant.h"

MessageClient::MessageClient(const char* serverName)
{
    managed_shared_memory *sharedMemory;
    try
    {
        sharedMemory = new managed_shared_memory(open_only, serverName);
    }
    catch (...)
    {
        throw "Server does not exist";
    }

    _ManagedShm = std::unique_ptr<managed_shared_memory>(sharedMemory);
    _Lock = _ManagedShm->find<bool>(LOCK).first;
    _Counter = _ManagedShm->find<U32>(COUNTER).first;
    _Queue = _ManagedShm->find<deque<U32>>(QUEUE).first;
}

MessageClient::~MessageClient()
{
}

Message* MessageClient::AllocateMessage(Message::Type type, const U32 &payloadSize)
{
    std::ostringstream stringStream;
    stringStream << MESSAGE;
    stringStream << *_Counter;

    void* payload = nullptr;
    boost::interprocess::managed_shared_memory::handle_t handle = 0;
    if (payloadSize)
    {
        payload = _ManagedShm->allocate(payloadSize, std::nothrow);
        handle = _ManagedShm->get_handle_from_address(payload);
    }

    Message* message = _ManagedShm->construct<Message>(stringStream.str().c_str())();
    message->_Id = *_Counter;
    message->_Type = type;
    message->_PayloadSize = payloadSize;
    message->_PayloadHandle = handle;
    message->_Payload = payload;
    (*_Counter)++;

    return message;
}

void MessageClient::Push(Message* message)
{
    *_Lock = true;
    _Queue->push_back(message->_Id);
    *_Lock = false;
}
