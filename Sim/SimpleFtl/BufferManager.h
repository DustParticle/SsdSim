#ifndef __BufferManager_h__
#define __BufferManager_h__

#include "HostComm/CustomProtocol/CustomProtocolInterface.h"
#include "Nand/Hal/NandHal.h"

class BufferManager
{
public:
    static U8* GetBuffer(const U32 &bufferSizeInSector);
    static void Memcpy(U8 *dest, U8 *src, const U32 &lengthInSector);
};

#endif