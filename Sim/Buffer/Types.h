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

struct Buffer
{
    BufferHandle Handle;
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