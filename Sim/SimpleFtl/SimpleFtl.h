#ifndef __SimpleFtl_h__
#define __SimpleFtl_h__

#include "HostComm/CustomProtocol/CustomProtocolInterface.h"
#include "Nand/Hal/NandHal.h"
#include "Translation.h"

class SimpleFtl
{
public:
    void SetProtocol(CustomProtocolInterface *interface);
    void SetNandHal(NandHal *nandHal);
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

	void ReadPage(const U32& lba, U8* outBuffer);
    void ReadPage(const U32& lba, U8 *outBuffer, const tSectorOffset& sectorOffset, const tSectorCount& sectorCount);
	void WritePage(const U32& lba, U8* inBuffer);
    void WritePage(const U32& lba, U8* inBuffer, const tSectorOffset& sectorOffset, const tSectorCount& sectorCount);

private:
    NandHal *_NandHal;
    CustomProtocolInterface *_CustomProtocolInterface;
    U32 _TotalSectors;
    U8 _SectorsPerPage;
};

#endif