#ifndef __Translation_h__
#define __Translation_h__

#include "Nand/Hal/NandHal.h"

namespace SimpleFtlTranslation
{
    inline void LbaToNandAddress(const NandHal::Geometry &geometry, const U32 &lba, NandHal::NandAddress &nandAddress)
    {
        U32 pageIndex = lba / (geometry._BytesPerPage >> 9);
        nandAddress.Channel._ = pageIndex % geometry._ChannelCount;
        nandAddress.Device._ = (pageIndex / geometry._ChannelCount) % geometry._DevicesPerChannel;
        nandAddress.Page._ = ((pageIndex / geometry._ChannelCount) / geometry._DevicesPerChannel) % geometry._PagesPerBlock;
        nandAddress.Block._ = (((pageIndex / geometry._ChannelCount) / geometry._DevicesPerChannel) / geometry._PagesPerBlock);
    }
}

#endif