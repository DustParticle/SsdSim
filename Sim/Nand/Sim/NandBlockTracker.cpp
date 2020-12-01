#include <cstring>
#include <assert.h>

#include "Nand/Sim/NandBlockTracker.h"

NandBlockTracker::NandBlockTracker(U32 pagesPerBlock)
{
    _PagesPerBlock = pagesPerBlock;
    _BitmapSize = CalculatingBitmapSize(pagesPerBlock);

    _CorruptedPagesBitmap = std::unique_ptr<U8[]>(new U8[_BitmapSize]);

    Reset();
}

void NandBlockTracker::Reset()
{
    _PageWrittenMarker = 0;
    std::memset(_CorruptedPagesBitmap.get(), 0, _BitmapSize);
}

void NandBlockTracker::WritePage(tPageInBlock page)
{
    if (page >= _PageWrittenMarker)
    {
        _PageWrittenMarker = page + 1;
    } else
    {
        // Mark the page is corrupted because this is write twice or write backward
        U32 byteIndex = page >> 3;
        U32 bitIndex = page & 7;
        _CorruptedPagesBitmap[byteIndex] |= (1 << bitIndex);
    }
}

bool NandBlockTracker::IsPageCorrupted(tPageInBlock page)
{
    U32 byteIndex = page >> 3;
    U32 bitIndex = page & 7;

    if (0 != (_CorruptedPagesBitmap[byteIndex] & (1 << bitIndex)))
    {
        return (true);
    }
    return (false);
}
