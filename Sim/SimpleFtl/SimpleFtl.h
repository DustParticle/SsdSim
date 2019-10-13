#ifndef __SimpleFtl_h__
#define __SimpleFtl_h__

#include "boost/lockfree/queue.hpp"

#include "Buffer/Hal/BufferHal.h"
#include "HostComm/CustomProtocol/CustomProtocolInterface.h"
#include "Nand/Hal/NandHal.h"
#include "Translation.h"

class SimpleFtl : public CustomProtocolInterface::TransferCommandListener, public NandHal::CommandListener
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
            CustomProtocolInterface::TransferCommandDesc TransferCommand;
            NandHal::CommandDesc NandCommand;
        };

        Type EventType;
        Params EventParams;
    };

public:
    SimpleFtl();

    void SetProtocol(CustomProtocolInterface *interface);
    void SetNandHal(NandHal *nandHal);
    void SetBufferHal(BufferHal *bufferHal);
    void operator()();

    void SubmitCustomProtocolCommand(CustomProtocolCommand *command);
    virtual void HandleCommandCompleted(const CustomProtocolInterface::TransferCommandDesc &command);
    virtual void HandleCommandCompleted(const NandHal::CommandDesc &command);

    bool IsProcessingCommand();

private:
	struct tSectorOffset
	{
		U8 _;
	};

	struct tSectorCount
	{
		U8 _;
	};

private:
    void ReadNextLbas();
    void TransferOut(const Buffer &buffer, const NandHal::NandAddress &nandAddress, const U32 &sectorIndex);
    void ReadPage(const NandHal::NandAddress &nandAddress, const Buffer &outBuffer, const U32 &descSectorIndex);

    void WriteNextLbas();
    void TransferIn(const Buffer &buffer, const NandHal::NandAddress &nandAddress, const U32 &sectorIndex);
	void WritePage(const NandHal::NandAddress &nandAddress, const Buffer &outBuffer);

    bool SetSectorInfo(const SectorInfo &sectorInfo);

    void OnNewCustomProtocolCommand(CustomProtocolCommand *command);
    void OnTransferCommandCompleted(const CustomProtocolInterface::TransferCommandDesc &command);
    void OnNandCommandCompleted(const NandHal::CommandDesc &command);

    void SubmitResponse();

private:
    NandHal *_NandHal;
    BufferHal *_BufferHal;
    CustomProtocolInterface *_CustomProtocolInterface;
    U32 _TotalSectors;
    U8 _SectorsPerPage;

    CustomProtocolCommand *_ProcessingCommand;
    U32 _RemainingSectorCount;
    U32 _ProcessedSectorCount;
    U32 _CurrentLba;
    U32 _PendingCommandCount;

    bip::interprocess_mutex *_Mutex;

    std::unique_ptr<boost::lockfree::queue<Event>> _EventQueue;
};

#endif