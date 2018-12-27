#include "Nand/NandHal.h"

void NandHal::PreInit(U8 chipCount, U32 blocksPerPage, U32 pagesPerBlock, U32 bytesPerPage)
{
	_ChipCount = chipCount;
	_BlocksPerChip = blocksPerPage;
	_PagesPerBlock = pagesPerBlock;
	_BytesPerPage = bytesPerPage;
}

void NandHal::Init()
{
	//Normally in hardware implementation we would query each chip 
	//Here we rely on PreInit to get configuration

	for (U8 i(0); i < _ChipCount; ++i)
	{
		_NandDevices.push_back(std::move(NandDevice(_BlocksPerChip, _PagesPerBlock, _BytesPerPage)));
	}
}

void NandHal::ReadPage(tChip chip, tBlockInChip block, tPageInBlock page, U8* const pOutData)
{
	_NandDevices[chip._].ReadPage(block, page, pOutData);
}

void NandHal::WritePage(tChip chip, tBlockInChip block, tPageInBlock page, const U8* const pInData)
{
	_NandDevices[chip._].WritePage(block, page, pInData);
}

void NandHal::EraseBlock(tChip chip, tBlockInChip block)
{
	_NandDevices[chip._].EraseBlock(block);
}

void NandHal::Run()
{
	//For now, hardcode NAND specifications
	//Later they will be "scanned"
	constexpr U8 chips = 1;
	constexpr U32 blocks = 128;
	constexpr U32 pages = 256;
	constexpr U32 bytes = 8192;
	PreInit(chips, blocks, pages, bytes);

	while (false == IsStopRequested())
	{
		//Later will add processing
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}