#include <assert.h>

#include "BufferHal.h"

BufferHal::BufferHal() : _CurrentBufferHandle(0)
{
    _AllocatedBuffers = std::make_unique<std::map<U32, std::unique_ptr<U8[]>>>();
}

void BufferHal::PreInit(const U32 &maxBufferSizeInKB)
{
    _MaxBufferSizeInSector = maxBufferSizeInKB * 2;
    _CurrentFreeSizeInSector = _MaxBufferSizeInSector;
}

bool BufferHal::AllocateBuffer(const U32 &bufferSizeInSector, Buffer &buffer)
{
    if (bufferSizeInSector > _CurrentFreeSizeInSector)
    {
        return false;
    }

    buffer.Handle = _CurrentBufferHandle;
    buffer.SizeInSector = bufferSizeInSector;

    _AllocatedBuffers->insert(std::pair<U32, std::unique_ptr<U8[]>>(_CurrentBufferHandle,
        std::make_unique<U8[]>(bufferSizeInSector << SectorSizeInBits)));

    ++_CurrentBufferHandle;
    _CurrentFreeSizeInSector -= bufferSizeInSector;

    return true;
}

void BufferHal::DeallocateBuffer(const Buffer &buffer)
{
    assert(buffer.SizeInSector + _CurrentFreeSizeInSector <= _MaxBufferSizeInSector);
    
    auto temp = _AllocatedBuffers->find(buffer.Handle);
    assert(_AllocatedBuffers->end() != temp);
    _AllocatedBuffers->erase(temp);
    _CurrentFreeSizeInSector += buffer.SizeInSector;
}

U8* BufferHal::ToPointer(const Buffer &buffer)
{
    auto temp = _AllocatedBuffers->find(buffer.Handle);
    if (temp != _AllocatedBuffers->end())
    {
        return temp->second.get();
    }
    return nullptr;
}

void BufferHal::Memcpy(U8* const dest, const Buffer &src)
{
    memcpy(dest, ToPointer(src), src.SizeInSector << SectorSizeInBits);
}

void BufferHal::Memcpy(const Buffer &dest, const U8* const src)
{
    memcpy(ToPointer(dest), src, dest.SizeInSector << SectorSizeInBits);
}
