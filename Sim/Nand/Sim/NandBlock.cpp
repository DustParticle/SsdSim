#include <cstring>
#include <assert.h>

#include "Nand/Sim/NandBlock.h"

NandBlock::NandBlock(BufferHal *bufferHal, U32 pagesPerBlock, U32 totalBytesPerPage) : _NandBlockTracker(pagesPerBlock)
{
	_PagesPerBlock = pagesPerBlock;
	_TotalBytesPerPage = totalBytesPerPage;
    _BufferHal = bufferHal;

	_ErasedBuffer = std::unique_ptr<U8[]>(new U8[_TotalBytesPerPage]);
	std::memset(_ErasedBuffer.get(), ERASED_PATTERN, _TotalBytesPerPage);
}

void NandBlock::Erase()
{
	_Buffer.reset();
	_NandBlockTracker.Reset();
}

void NandBlock::WritePage(tPageInBlock page, const Buffer &inBuffer)
{
	if (nullptr == _Buffer)
	{
		_Buffer = std::unique_ptr<U8[]>(new U8[_PagesPerBlock * _TotalBytesPerPage]);
	}

    auto sectorsPerPage = _TotalBytesPerPage >> _BufferHal->GetSectorInfo().SectorSizeInBit;
    tSectorCount sectorCount;
    sectorCount._ = sectorsPerPage;
    tSectorOffset sectorOffset;
    sectorOffset._ = 0;
    _BufferHal->Memcpy((U8*)&_Buffer[page._ * _TotalBytesPerPage], inBuffer, sectorOffset, sectorCount);
	_NandBlockTracker.WritePage(page);
}

void NandBlock::WritePage(const tPageInBlock& page, const tSectorInPage& sector, const tSectorCount& sectorCount, const Buffer &inBuffer, const tSectorOffset& bufferOffset)
{
	assert(sector._ >= 0);
	assert(_BufferHal->ToByteIndexInTransfer(inBuffer.Type, sector._ + sectorCount._) <= _TotalBytesPerPage);

	if (nullptr == _Buffer)
	{
		_Buffer = std::unique_ptr<U8[]>(new U8[_PagesPerBlock * _TotalBytesPerPage]);
	}
    
    _BufferHal->Memcpy((U8*)&_Buffer[page._ * _TotalBytesPerPage + _BufferHal->ToByteIndexInTransfer(inBuffer.Type, sector._)], inBuffer, bufferOffset, sectorCount);
	_NandBlockTracker.WritePage(page);
}

bool NandBlock::ReadPage(tPageInBlock page, const Buffer &outBuffer)
{
    // If page is corrupted then return false to indicate ReadPage failed
    if (true == _NandBlockTracker.IsPageCorrupted(page))
    {
        return (false);
    }

	auto pData = (nullptr == _Buffer) ? &_ErasedBuffer[0] : &_Buffer[page._ * _TotalBytesPerPage];

    auto sectorsPerPage = _TotalBytesPerPage >> _BufferHal->GetSectorInfo().SectorSizeInBit;
    tSectorCount sectorCount;
    sectorCount._ = sectorsPerPage;
    tSectorOffset sectorOffset;
    sectorOffset._ = 0;
    _BufferHal->Memcpy(outBuffer, sectorOffset, pData, sectorCount);
	return (true);
}

bool NandBlock::ReadPage(const tPageInBlock& page, const tSectorInPage& sector, const tSectorCount& sectorCount, const Buffer &outBuffer, const tSectorOffset& bufferOffset)
{
    // If page is corrupted then return false to indicate ReadPage failed
    if (true == _NandBlockTracker.IsPageCorrupted(page))
    {
        return (false);
    }

	assert(sector._ >= 0);
	assert(_BufferHal->ToByteIndexInTransfer(outBuffer.Type, sector._ + sectorCount._) <= _TotalBytesPerPage);

	auto data = (nullptr == _Buffer) ? &_ErasedBuffer[0] : &_Buffer[(page._ * _TotalBytesPerPage) + _BufferHal->ToByteIndexInTransfer(outBuffer.Type, sector._)];

    _BufferHal->Memcpy(outBuffer, bufferOffset, data, sectorCount);
	return (true);
}