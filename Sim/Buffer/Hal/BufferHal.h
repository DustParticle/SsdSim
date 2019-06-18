#ifndef __BufferHal_h__
#define __BufferHal_h__

#include "BasicTypes.h"
#include "Buffer/Types.h"

class BufferHal
{
public:
    BufferHal();

    void PreInit(const U32 &maxBufferSize);

public:
    Buffer AllocateBuffer(const U32 &bufferSize);
    void DeallocateBuffer(const Buffer &buffer);

    U8* ToBuffer(const Buffer &buffer);
    void Memcpy(U8* const dest, const Buffer &src);
    void Memcpy(const Buffer &dest, const U8* const src);

private:
    std::unique_ptr<U8[]> _SharedBuffer;
};

#endif