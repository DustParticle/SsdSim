#ifndef __SimpleFtl_h__
#define __SimpleFtl_h__

#include "boost/lockfree/queue.hpp"

#include "Buffer/Hal/BufferHal.h"
#include "HostComm/CustomProtocol/CustomProtocolHal.h"
#include "Nand/Hal/NandHal.h"
#include "Translation.h"

class SimpleFtl : public CustomProtocolHal::TransferCommandListener, public NandHal::CommandListener
{
private:
    enum State
    {
        PROCESSING,
        IDLE
    };

    struct Event
    {
        enum class Type
        {
            CustomProtocolCommand,
            TransferCompleted,
            NandCommandCompleted
        };

        union Params
        {
            CustomProtocolCommand *CustomProtocolCommand;
            CustomProtocolHal::TransferCommandDesc TransferCommand;
            NandHal::CommandDesc NandCommand;
        };

        Type EventType;
        Params EventParams;
    };

public:
    SimpleFtl();

    void SetProtocol(CustomProtocolHal *customProtocolHal);
    void SetNandHal(NandHal *nandHal);
    void SetBufferHal(BufferHal *bufferHal);
    void Shutdown();
    void operator()();

    void SubmitCustomProtocolCommand(CustomProtocolCommand *command);
    virtual void HandleCommandCompleted(const CustomProtocolHal::TransferCommandDesc &command);
    virtual void HandleCommandCompleted(const NandHal::CommandDesc &command);

    bool IsProcessingCommand();

private:
    void ProcessEvent();
    void ReadNextLbas();
    void TransferOut(const Buffer &buffer, const tSectorOffset& bufferOffset, const tSectorOffset& commandOffset, const tSectorCount& sectorCount);
    void ReadPage(const NandHal::NandAddress &nandAddress, const Buffer &outBuffer, const tSectorOffset& descSectorIndex);

    void HandleWriteCommand();
    void AllocateNextBuffer(Buffer &buffer);
    void HandleNandReadAndTransferCompleted();
    void HandleNandEraseCompleted();
    bool IsSameBlock(const NandHal::NandAddress& nandAddress1, const NandHal::NandAddress& nandAddress2);
    void EraseBlock(const NandHal::NandAddress& nandAddress);
    void TransferIn(const Buffer &buffer, const tSectorOffset& bufferOffset, const tSectorOffset& commandOffset, const tSectorCount& sectorCount);
	void WritePage(const NandHal::NandAddress &nandAddress, const Buffer &outBuffer);

    bool SetSectorInfo(const SectorInfo &sectorInfo);

    void OnNewCustomProtocolCommand(CustomProtocolCommand *command);
    void OnTransferCommandCompleted(const CustomProtocolHal::TransferCommandDesc &command);
    void OnNandCommandCompleted(const NandHal::CommandDesc &command);

    void SubmitResponse();

private:
    NandHal *_NandHal;
    BufferHal *_BufferHal;
    CustomProtocolHal *_CustomProtocolHal;
    U32 _TotalSectors;
    U8 _SectorsPerPage;
    U8 _SectorsPerSegment;
    U32 _PagesPerBlock;

    Buffer* _BlockBuffers;
    NandHal::NandAddress *_ProcessingBlocks;
    U32 _BlockBufferCount;
    U32 _ProcessingBlockCount;

    CustomProtocolCommand *_ProcessingCommand;
    tSectorOffset _ProcessedSectorOffset;
    U32 _RemainingSectorCount;
    U32 _CurrentLba;
    U32 _PendingTransferCommandCount;
    U32 _PendingNandCommandCount;

    NandHal::NandAddress _ProcessingPage;

    bip::interprocess_mutex *_Mutex;

    std::unique_ptr<boost::lockfree::queue<Event>> _EventQueue;
};

#endif