#ifndef __Translation_h__
#define __Translation_h__

#include "Nand/Hal/NandHal.h"

namespace SimpleFtlTranslation
{
    inline void LbaToNandAddress(const NandHal::Geometry &geometry, const U32 &lba, NandHal::CommandDesc &commandDesc)
    {
        U32 pageIndex = lba / (geometry._BytesPerPage >> 9);
        commandDesc.Channel._ = pageIndex % geometry._ChannelCount;
        commandDesc.Device._ = (pageIndex / geometry._ChannelCount) % geometry._DevicesPerChannel;
        commandDesc.Page._ = ((pageIndex / geometry._ChannelCount) / geometry._DevicesPerChannel) % geometry._PagesPerBlock;
        commandDesc.Block._ = (((pageIndex / geometry._ChannelCount) / geometry._DevicesPerChannel) / geometry._PagesPerBlock);
    }
}

#endif