#ifndef __SimpleFtl_h__
#define __SimpleFtl_h__

#include "Buffer/Hal/BufferHal.h"
#include "HostComm/CustomProtocol/CustomProtocolInterface.h"
#include "Nand/Hal/NandHal.h"
#include "Translation.h"

class SimpleFtl
{
public:
    void SetProtocol(CustomProtocolInterface *interface);
    void SetNandHal(NandHal *nandHal);
    void SetBufferHal(BufferHal *bufferHal);
    void operator()();

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
    void ReadFromNand(CustomProtocolCommand *command);
    void WriteToNand(CustomProtocolCommand *command);

	void ReadPage(const U32& lba, const Buffer &outBuffer, const U32 &descSectorIndex);
    void ReadPage(const U32& lba, const Buffer &outBuffer, const tSectorOffset& sectorOffset, const tSectorCount& sectorCount, const U32 &descSectorIndex);
	void WritePage(const U32& lba, const Buffer &outBuffer);
    void WritePage(const U32& lba, const Buffer &outBuffer, const tSectorOffset& sectorOffset, const tSectorCount& sectorCount);

private:
    NandHal *_NandHal;
    BufferHal *_BufferHal;
    CustomProtocolInterface *_CustomProtocolInterface;
    U32 _TotalSectors;
    U8 _SectorsPerPage;
};

#endif