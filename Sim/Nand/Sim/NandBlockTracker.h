#ifndef __NandBlock_h__
#define __NandBlock_h__

#include <memory>

#include "Nand/Types.h"

class NandBlockTracker
{
public:
    NandBlockTracker(U32 pagesPerBlock);
    NandBlockTracker(NandBlock&& rhs) = default;

public:
    void Reset();
    void WritePage(tPageInBlock page);
    bool IsPageWritten(tPageInBlock page);

public:
    static const U8 BYTE_SIZE = 8;

private:
    U32 _PagesPerBlock;
    U32 _BitmapSize;
    tPageInBlock _PageWrittenMarker;

    std::unique_ptr<U8[]> _PageWrittenBitmap;
};

#endif
