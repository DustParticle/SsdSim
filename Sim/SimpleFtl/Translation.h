#ifndef __Translation_h__
#define __Translation_h__

#include "Nand/Hal/NandHal.h"

namespace SimpleFtlTranslation
{
    constexpr U8 SectorSizeInBits = 9;
    constexpr U16 SectorSizeInBytes = 1 << SectorSizeInBits;        // 512

    static NandHal::Geometry _Geometry;

    static inline void LbaToNandAddress(const U32 &lba, NandHal::NandAddress &nandAddress)
    {
        U32 pageIndex = lba / (_Geometry._BytesPerPage >> SectorSizeInBits);
        nandAddress.Channel._ = pageIndex % _Geometry._ChannelCount;
        nandAddress.Device._ = (pageIndex / _Geometry._ChannelCount) % _Geometry._DevicesPerChannel;
        nandAddress.Page._ = ((pageIndex / _Geometry._ChannelCount) / _Geometry._DevicesPerChannel) % _Geometry._PagesPerBlock;
        nandAddress.Block._ = (((pageIndex / _Geometry._ChannelCount) / _Geometry._DevicesPerChannel) / _Geometry._PagesPerBlock);
    }

    static inline void SetGeometry(const NandHal::Geometry &geometry)
    {
        _Geometry = geometry;
    }
};

#endif