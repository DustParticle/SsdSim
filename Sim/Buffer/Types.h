#ifndef __BufferTypes_h__
#define __BufferTypes_h__

#include "BasicTypes.h"

enum BufferType
{
    User,
    System,
    Undefined
};

struct BufferHandle
{
    U32 _;
};

using tBufferHandle = PrimitiveTemplate<U32, struct BufferHandle>;

struct Buffer
{
    tBufferHandle Handle;
    U32 SizeInSector;
    U32 SizeInByte;
    BufferType Type;
};

struct SectorInfo
{
    U8 SectorSizeInBit;
    bool CompactMode;
    U16 CompactSizeInByte;
};

#endif