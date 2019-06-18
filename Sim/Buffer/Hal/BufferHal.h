#ifndef __BufferHal_h__
#define __BufferHal_h__

#include <map>

#include "BasicTypes.h"
#include "Buffer/Types.h"

constexpr U8 SectorSizeInBits = 9;

class BufferHal
{
public:
    BufferHal();

    void PreInit(const U32 &maxBufferSizeInKB);

public:
    bool AllocateBuffer(const U32 &sectorCount, Buffer &buffer);
    void DeallocateBuffer(const Buffer &buffer);

    U8* ToBuffer(const Buffer &buffer);
    void Memcpy(U8* const dest, const Buffer &src);
    void Memcpy(const Buffer &dest, const U8* const src);

private:
    U32 _MaxBufferSizeInSector;
    U32 _CurrentFreeSizeInSector;
    U32 _CurrentBufferHandle;
    std::unique_ptr<std::map<U32, std::unique_ptr<U8[]>>> _AllocatedBuffers;
};

#endif