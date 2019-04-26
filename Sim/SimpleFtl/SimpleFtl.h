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
    void ReadFromNand(CustomProtocolCommand *command);
    void WriteToNand(CustomProtocolCommand *command);

    bool ReadPage(const U32 &lba, U8 *outBuffer, const U8 &startSectorIndex, const U8 &sectorToRead);
    bool WritePage(const U32 &lba, U8 *inBuffer);

    void CopySectors(U8 *dest, U8 *src, const U32 &lengthInSector);

private:
    NandHal *_NandHal;
    CustomProtocolInterface *_CustomProtocolInterface;
    U32 _TotalSectors;
    U8 _SectorsPerPage;
    U8 _SharedBuffer[SimpleFtlTranslation::SectorSizeInBytes * 100];
};

#endif