#include <cstring>
#include <assert.h>

#include "Nand/Sim/NandBlockTracker.h"

NandBlockTracker::NandBlockTracker(U32 pagesPerBlock)
{
    _PagesPerBlock = pagesPerBlock;
    _BitmapSize = CalculatingBitmapSize(pagesPerBlock);

    _PageWrittenBitmap = std::unique_ptr<U8[]>(new U8[_BitmapSize]);

    Reset();
}

void NandBlockTracker::Reset()
{
    _PageWrittenMarker._ = 0;
    std::memset(_PageWrittenBitmap.get(), 0, _BitmapSize);
}

void NandBlockTracker::WritePage(tPageInBlock page)
{
    if (page._ >= _PageWrittenMarker._)
    {
        _PageWrittenMarker._ = page._ + 1;
    } else
    {
        // Mark the page is corrupted because this is write twice or write backward
        U32 byteIndex = page._ >> 3;
        U32 bitIndex = page._ & 7;
        _PageWrittenBitmap[byteIndex] |= (1 << bitIndex);
    }
}

bool NandBlockTracker::IsPageWritten(tPageInBlock page)
{
    U32 byteIndex = page._ >> 3;
    U32 bitIndex = page._ & 7;

    if (0 != (_PageWrittenBitmap[byteIndex] & (1 << bitIndex)))
    {
        return (true);
    }
    return (false);
}
