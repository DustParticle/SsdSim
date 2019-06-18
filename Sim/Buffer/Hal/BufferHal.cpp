#include "BufferHal.h"

BufferHal::BufferHal()
{
}

void BufferHal::PreInit(const U32 &maxBufferSize)
{
    _SharedBuffer = std::make_unique<U8[]>(maxBufferSize);
}

Buffer BufferHal::AllocateBuffer(const U32 &bufferSizeInSector)
{
    Buffer buffer;
    buffer.Index = 0;
    buffer.Size = bufferSizeInSector;

    return buffer;
}

void BufferHal::DeallocateBuffer(const Buffer &buffer)
{
    
}

U8* BufferHal::ToBuffer(const Buffer &buffer)
{
    return &_SharedBuffer[buffer.Index];
}

void BufferHal::Memcpy(U8* const dest, const Buffer &src)
{
    memcpy(dest, ToBuffer(src), src.Size);
}

void BufferHal::Memcpy(const Buffer &dest, const U8* const src)
{
    memcpy(ToBuffer(dest), src, dest.Size);
}
