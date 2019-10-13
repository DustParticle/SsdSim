#ifndef __CustomProtocolInterface_h__
#define __CustomProtocolInterface_h__

#include "boost/lockfree/spsc_queue.hpp"

#include "SimFrameworkBase/FrameworkThread.h"

#include "HostComm/BasicTypes.h"
#include "CustomProtocolCommand.h"
#include "HostComm/Ipc/MessageServer.hpp"
#include "Buffer/Hal/BufferHal.h"
#include "Nand/Hal/NandHal.h"

class CustomProtocolInterface : public FrameworkThread
{
public:
    CustomProtocolInterface();
    
    void Init(const char *protocolIpcName = nullptr, BufferHal *bufferHal = nullptr);

    bool HasCommand();
    CustomProtocolCommand* GetCommand();
    void SubmitResponse(CustomProtocolCommand *command);

public:
    struct TransferCommandDesc;
    class TransferCommandListener
    {
    public:
        virtual void HandleCommandCompleted(const TransferCommandDesc &command) = 0;
    };

    struct TransferCommandDesc
    {
        enum class Direction
        {
            In,
            Out
        };

        CustomProtocolCommand *Command;
        U32 SectorIndex;

        Direction Direction;
        Buffer Buffer;
        NandHal::NandAddress NandAddress;
        TransferCommandListener *Listener;
    };

public:
    void QueueCommand(const TransferCommandDesc &command);

protected:
    virtual void Run() override;

private:
    U8* GetBuffer(CustomProtocolCommand *command, const U32 &sectorIndex);

private:
    std::unique_ptr<MessageServer<CustomProtocolCommand>> _MessageServer;
    BufferHal *_BufferHal;

    std::unique_ptr<boost::lockfree::spsc_queue<TransferCommandDesc>> _TransferCommandQueue;
};

#endif