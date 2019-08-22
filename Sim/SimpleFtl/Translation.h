#ifndef __Translation_h__
#define __Translation_h__

#include "Nand/Hal/NandHal.h"

namespace SimpleFtlTranslation
{
    static U8 _SectorSizeInBit;
    static U8 _SectorsPerPage;

    static NandHal::Geometry _Geometry;

    static inline void LbaToNandAddress(const U32 &lba, const U32& sectorCount,
        NandHal::NandAddress &nandAddress, U32 &nextLba, U32 &remainSectorCount)
    {
        U32 pageIndex = lba / (_Geometry.BytesPerPage >> _SectorSizeInBit);
        nandAddress.Channel._ = pageIndex % _Geometry.ChannelCount;
        nandAddress.Device._ = (pageIndex / _Geometry.ChannelCount) % _Geometry.DevicesPerChannel;
        nandAddress.Page._ = ((pageIndex / _Geometry.ChannelCount) / _Geometry.DevicesPerChannel) % _Geometry.PagesPerBlock;
        nandAddress.Block._ = (((pageIndex / _Geometry.ChannelCount) / _Geometry.DevicesPerChannel) / _Geometry.PagesPerBlock);
        nandAddress.Sector._ = lba % _SectorsPerPage;

        if (nandAddress.Sector._ + sectorCount <= _SectorsPerPage)
        {
            nandAddress.SectorCount._ = sectorCount;
        }
        else
        {
            nandAddress.SectorCount._ = _SectorsPerPage - nandAddress.Sector._;
        }

        nextLba = lba + nandAddress.SectorCount._;
        remainSectorCount = sectorCount - nandAddress.SectorCount._;
    }

    static inline void SetGeometry(const NandHal::Geometry &geometry)
    {
        _Geometry = geometry;
    }

    static inline void SetSectorSize(const U8 &sectorSizeInBit)
    {
        _SectorSizeInBit = sectorSizeInBit;
        _SectorsPerPage = _Geometry.BytesPerPage >> _SectorSizeInBit;
    }
};

#endif