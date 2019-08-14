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
    bool IsPageCorrupted(tPageInBlock page);

private:
    constexpr U32 CalculatingBitmapSize(U32 papgesPerBlock)
    {
        return ((papgesPerBlock + (8 - 1)) / 8);
    }

private:
    U32 _PagesPerBlock;
    U32 _BitmapSize;
    tPageInBlock _PageWrittenMarker;

    std::unique_ptr<U8[]> _CorruptedPagesBitmap;
};

#endif
