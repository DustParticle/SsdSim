#include "BufferManager.h"

#include "Translation.h"

U8 SharedBuffer[SimpleFtlTranslation::SectorSizeInBytes * 100];

U8* BufferManager::GetBuffer(const U32 &bufferSizeInSector)
{
    return SharedBuffer;
}

void BufferManager::Memcpy(U8 *dest, U8 *src, const U32 &lengthInSector)
{
    memcpy(dest, src, lengthInSector << SimpleFtlTranslation::SectorSizeInBits);
}
