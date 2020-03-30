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
    void operator()();

    void SubmitCustomProtocolCommand(CustomProtocolCommand *command);
    virtual void HandleCommandCompleted(const CustomProtocolHal::TransferCommandDesc &command);
    virtual void HandleCommandCompleted(const NandHal::CommandDesc &command);

    bool IsProcessingCommand();

private:
    void ProcessEvent();
    void ReadNextLbas();
    void TransferOut(const Buffer &buffer, const NandHal::NandAddress &nandAddress, const tSectorOffset& commandOffset, const tSectorCount& sectorCount);
    void ReadPage(const NandHal::NandAddress &nandAddress, const Buffer &outBuffer, const tSectorOffset& descSectorIndex);

    void WriteNextLbas();
    void TransferIn(const Buffer &buffer, const NandHal::NandAddress &nandAddress, const tSectorOffset& commandOffset, const tSectorCount& sectorCount);
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

    CustomProtocolCommand *_ProcessingCommand;
    U32 _RemainingSectorCount;
    U32 _ProcessedSectorCount;
    U32 _CurrentLba;
    U32 _PendingCommandCount;

    U8 _SectorsPerSegment;

    bip::interprocess_mutex *_Mutex;

    std::unique_ptr<boost::lockfree::queue<Event>> _EventQueue;
};

#endif