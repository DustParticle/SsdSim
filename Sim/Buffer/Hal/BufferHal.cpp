#include <assert.h>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include "BufferHal.h"

using namespace boost::interprocess;

BufferHal::BufferHal() : _CurrentBufferHandle(0), _SectorInfo(DefaultSectorInfo)
{
    _AllocatedBuffers = std::make_unique<std::map<U32, std::unique_ptr<U8[]>>>();
    SetImplicitAllocationSectorCount(1);
}

void BufferHal::PreInit(const U32 &maxBufferSizeInKB)
{
    _MaxBufferSizeInSector = maxBufferSizeInKB * 2;
    _CurrentFreeSizeInSector = _MaxBufferSizeInSector;
}

void BufferHal::SetImplicitAllocationSectorCount(const U32& sectorCount)
{
    _ImplicitAllocationSectorCount = sectorCount;
}

bool BufferHal::AllocateBuffer(BufferType type, const U32 &bufferSizeInSector, Buffer &buffer)
{
    scoped_lock<interprocess_mutex> lock(_Mutex);
    if (bufferSizeInSector > _CurrentFreeSizeInSector)
    {
        return false;
    }

    buffer.Handle = _CurrentBufferHandle;
    buffer.Type = type;
    buffer.SizeInSector = bufferSizeInSector;
    buffer.SizeInByte = ToByteIndexInTransfer(type, bufferSizeInSector);

    _AllocatedBuffers->insert(std::pair<U32, std::unique_ptr<U8[]>>(_CurrentBufferHandle,
        std::make_unique<U8[]>(buffer.SizeInByte)));

    ++_CurrentBufferHandle;
    _CurrentFreeSizeInSector -= bufferSizeInSector;

    return true;
}

bool BufferHal::AllocateBuffer(BufferType type, Buffer& buffer)
{
    return AllocateBuffer(type, _ImplicitAllocationSectorCount, buffer);
}

void BufferHal::DeallocateBuffer(const Buffer &buffer)
{
    scoped_lock<interprocess_mutex> lock(_Mutex);

    assert(buffer.SizeInSector + _CurrentFreeSizeInSector <= _MaxBufferSizeInSector);

    auto temp = _AllocatedBuffers->find(buffer.Handle);
    assert(_AllocatedBuffers->end() != temp);
    _AllocatedBuffers->erase(temp);
    _CurrentFreeSizeInSector += buffer.SizeInSector;
}

U8* BufferHal::ToPointer(const Buffer &buffer)
{
    scoped_lock<interprocess_mutex> lock(_Mutex);

    auto temp = _AllocatedBuffers->find(buffer.Handle);
    if (temp != _AllocatedBuffers->end())
    {
        return temp->second.get();
    }
    return nullptr;
}

void BufferHal::CopyFromBuffer(U8* const dest, const Buffer& buffer, const tSectorOffset& bufferOffset, const tSectorCount& sectorCount)
{
    auto byteOffset = ToByteIndexInTransfer(buffer.Type, bufferOffset._);
    auto byteCount = ToByteIndexInTransfer(buffer.Type, sectorCount._);
    memcpy(dest, ToPointer(buffer) + byteOffset, byteCount);
}

void BufferHal::CopyToBuffer(const U8* const src, const Buffer& buffer, const tSectorOffset& bufferOffset, const tSectorCount& sectorCount)
{
    auto byteOffset = ToByteIndexInTransfer(buffer.Type, bufferOffset._);
    auto byteCount = ToByteIndexInTransfer(buffer.Type, sectorCount._);
    memcpy(ToPointer(buffer) + byteOffset, src, byteCount);
}

bool BufferHal::SetSectorInfo(const SectorInfo &sectorInfo)
{
    if (sectorInfo.CompactMode == true && (1 << sectorInfo.SectorSizeInBit) < sectorInfo.CompactSizeInByte)
    {
        return false;
    }
    _SectorInfo = sectorInfo;
    return true;
}

SectorInfo BufferHal::GetSectorInfo() const
{
    return _SectorInfo;
}

U32 BufferHal::ToByteIndexInTransfer(BufferType type, U32 offset)
{
    return (type == BufferType::User && _SectorInfo.CompactMode)
        ? offset * _SectorInfo.CompactSizeInByte
        : offset << _SectorInfo.SectorSizeInBit;
}