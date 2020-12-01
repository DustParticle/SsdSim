#include "Nand/Sim/NandDevice.h"

NandDevice::NandDevice(BufferHal *bufferHal, U32 blockCount, U32 pagesPerBlock, U32 bytesPerPage)
{
	_Desc = std::unique_ptr<NandDeviceDesc>(new NandDeviceDesc(blockCount, pagesPerBlock, bytesPerPage));
	for (U32 i = 0; i < blockCount; ++i)
	{
		_Blocks.push_back(std::move(NandBlock(bufferHal, _Desc->GetPagesPerBlock(), _Desc->GetBytesPerPage())));
	}
}

bool NandDevice::ReadPage(tBlockInDevice block, tPageInBlock page, const Buffer &outBuffer)
{
	return (_Blocks[block].ReadPage(page, outBuffer));
}

bool NandDevice::ReadPage(const tBlockInDevice& block, const tPageInBlock& page, const tSectorInPage& sector, const tSectorCount& sectorCount, const Buffer &outBuffer, const tSectorOffset& bufferOffset)
{
	return (_Blocks[block].ReadPage(page, sector, sectorCount, outBuffer, bufferOffset));
}

void NandDevice::WritePage(tBlockInDevice block, tPageInBlock page, const Buffer &inBuffer)
{
	_Blocks[block].WritePage(page, inBuffer);
}

void NandDevice::WritePage(const tBlockInDevice& block, const tPageInBlock& page, const tSectorInPage& sector, const tSectorCount& sectorCount, const Buffer &inBuffer, const tSectorOffset& bufferOffset)
{
	_Blocks[block].WritePage(page, sector, sectorCount, inBuffer, bufferOffset);
}

void NandDevice::EraseBlock(tBlockInDevice block)
{
	_Blocks[block].Erase();
}