#include <assert.h>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include "BufferHal.h"

using namespace boost::interprocess;

BufferHal::BufferHal() : _CurrentBufferHandle(0), _SectorInfo(DefaultSectorInfo)
{
    _AllocatedBuffers = std::make_unique<std::map<U32, std::unique_ptr<U8[]>>>();
}

void BufferHal::PreInit(const U32 &maxBufferSizeInKB)
{
    _MaxBufferSizeInSector = maxBufferSizeInKB * 2;
    _CurrentFreeSizeInSector = _MaxBufferSizeInSector;
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

void BufferHal::Memcpy(U8* const dest, const Buffer &src, const tSectorOffset& bufferOffset, const tSectorCount& sectorCount)
{
    auto byteOffset = ToByteIndexInTransfer(src.Type, bufferOffset._);
    auto byteCount = ToByteIndexInTransfer(src.Type, sectorCount._);
    memcpy(dest, ToPointer(src) + byteOffset, byteCount);
}

void BufferHal::Memcpy(const Buffer &dest, const tSectorOffset& bufferOffset, const U8* const src, const tSectorCount& sectorCount)
{
    auto byteOffset = ToByteIndexInTransfer(dest.Type, bufferOffset._);
    auto byteCount = ToByteIndexInTransfer(dest.Type, sectorCount._);
    memcpy(ToPointer(dest) + byteOffset, src, byteCount);
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