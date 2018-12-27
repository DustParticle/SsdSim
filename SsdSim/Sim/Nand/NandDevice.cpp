#include "Nand/NandDevice.h"

NandDevice::NandDevice(U32 blockCount, U32 pagesPerBlock, U32 bytesPerPage)
{
	_Desc = std::unique_ptr<NandDeviceDesc>(new NandDeviceDesc(blockCount, pagesPerBlock, bytesPerPage));
	for (U32 i = 0; i < blockCount; ++i)
	{
		_Blocks.push_back(std::move(NandBlock(_Desc->GetPagesPerBlock(), _Desc->GetBytesPerPage())));
	}
}

void NandDevice::ReadPage(tBlockInChip block, tPageInBlock page, U8* const pOutData)
{
	_Blocks[block._].ReadPage(page, pOutData);
}

void NandDevice::WritePage(tBlockInChip block, tPageInBlock page, const U8* const pInData)
{
	_Blocks[block._].WritePage(page, pInData);
}

void NandDevice::EraseBlock(tBlockInChip block)
{
	_Blocks[block._].Erase();
}