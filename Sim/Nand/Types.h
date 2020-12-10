#ifndef __Types_h__
#define __Types_h__

#include "BasicTypes.h"

using tChannel = PrimitiveTemplate<U8, struct Channel>;
using tDeviceInChannel = PrimitiveTemplate<U8, struct DeviceInChannel>;
using tBlockInDevice = PrimitiveTemplate<U16, struct BlockInDevice>;
using tPageInBlock = PrimitiveTemplate<U16, struct PageInBlock>;
using tSectorInPage = PrimitiveTemplate<U8, struct SectorInPage>;

#endif