#ifndef __CustomProtocolInterface_h__
#define __CustomProtocolInterface_h__

#include "boost/lockfree/spsc_queue.hpp"

#include "SimFrameworkBase/FrameworkThread.h"

#include "HostComm/BasicTypes.h"
#include "CustomProtocolCommand.h"
#include "HostComm/Ipc/MessageServer.hpp"
#include "Buffer/Hal/BufferHal.h"
#include "Nand/Hal/NandHal.h"

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
};

class CustomProtocolInterface : public FrameworkThread
{
public:
    CustomProtocolInterface();
    
    void Init(const char *protocolIpcName = nullptr, BufferHal *bufferHal = nullptr);

    bool HasCommand();
    CustomProtocolCommand* GetCommand();
    void SubmitResponse(CustomProtocolCommand *command);

    void QueueCommand(const TransferCommandDesc &command);
    bool PopFinishedCommand(TransferCommandDesc& command);
    void ProcessTransfer();

protected:
    virtual void Run() override;

private:
    U8* GetBuffer(CustomProtocolCommand *command, const U32 &sectorIndex);

private:
    std::unique_ptr<MessageServer<CustomProtocolCommand>> _MessageServer;
    BufferHal *_BufferHal;

    std::unique_ptr<boost::lockfree::spsc_queue<TransferCommandDesc>> _TransferCommandQueue;
    std::unique_ptr<boost::lockfree::spsc_queue<TransferCommandDesc>> _FinishedTransferCommandQueue;
};

#endif