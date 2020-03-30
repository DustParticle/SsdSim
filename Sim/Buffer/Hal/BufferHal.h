#ifndef __BufferHal_h__
#define __BufferHal_h__

#include <map>
#include <boost/interprocess/sync/interprocess_mutex.hpp>

#include "BasicTypes.h"
#include "Buffer/Types.h"

constexpr SectorInfo DefaultSectorInfo({ 9, false, 9 });

class BufferHal
{
public:
public:
    BufferHal();

    void PreInit(const U32 &maxBufferSizeInKB);

public:
    bool AllocateBuffer(BufferType type, const U32 &sectorCount, Buffer &buffer);
    void DeallocateBuffer(const Buffer &buffer);

    U8* ToPointer(const Buffer &buffer);
    void Memcpy(U8* const dest, const Buffer &src, const tSectorOffset& bufferOffset, const tSectorCount& sectorCount);
    void Memcpy(const Buffer &dest, const tSectorOffset& bufferOffset, const U8* const src, const tSectorCount& sectorCount);

public:
    bool SetSectorInfo(const SectorInfo &sectorInfo);
    SectorInfo GetSectorInfo() const;
    U32 ToByteIndexInTransfer(BufferType type, U32 offset);

private:
    U32 _MaxBufferSizeInSector;
    U32 _CurrentFreeSizeInSector;
    U32 _CurrentBufferHandle;
    std::unique_ptr<std::map<U32, std::unique_ptr<U8[]>>> _AllocatedBuffers;
    SectorInfo _SectorInfo;

    boost::interprocess::interprocess_mutex _Mutex;
};

#endif