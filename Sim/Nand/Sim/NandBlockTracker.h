#ifndef __NandBlockTracker_h__
#define __NandBlockTracker_h__

#include <memory>

#include "Nand/Types.h"

class NandBlockTracker
{
public:
    NandBlockTracker(U32 pagesPerBlock);
    NandBlockTracker(NandBlockTracker&& rhs) = default;

public:
    void Reset();
    void WritePage(tPageInBlock page);
    bool IsPageWritten(tPageInBlock page);

public:
    constexpr U8 BYTE_SIZE = 8;

private:
    U32 _PagesPerBlock;
    U32 _BitmapSize;
    tPageInBlock _PageWrittenMarker;

    std::unique_ptr<U8[]> _PageWrittenBitmap;
};

#endif
