#include "Nand/Sim/NandDevice.h"

NandDevice::NandDevice(U32 blockCount, U32 pagesPerBlock, U32 bytesPerPage)
{
	_Desc = std::unique_ptr<NandDeviceDesc>(new NandDeviceDesc(blockCount, pagesPerBlock, bytesPerPage));
	for (U32 i = 0; i < blockCount; ++i)
	{
		_Blocks.push_back(std::move(NandBlock(_Desc->GetPagesPerBlock(), _Desc->GetBytesPerPage())));
	}
}

void NandDevice::ReadPage(tBlockInDevice block, tPageInBlock page, U8* const pOutData)
{
	_Blocks[block._].ReadPage(page, pOutData);
}

void NandDevice::ReadPage(const tBlockInDevice& block, const tPageInBlock& page, const tByteOffset& byteOffset, const tByteCount& byteCount, U8* const outBuffer)
{
	_Blocks[block._].ReadPage(page, byteOffset, byteCount, outBuffer);
}

void NandDevice::WritePage(tBlockInDevice block, tPageInBlock page, const U8* const pInData)
{
	_Blocks[block._].WritePage(page, pInData);
}

void NandDevice::WritePage(const tBlockInDevice& block, const tPageInBlock& page, const tByteOffset& byteOffset, const tByteCount& byteCount, const U8* const inBuffer)
{
	_Blocks[block._].WritePage(page, byteOffset, byteCount, inBuffer);
}

void NandDevice::EraseBlock(tBlockInDevice block)
{
	_Blocks[block._].Erase();
}