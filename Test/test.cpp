#include "pch.h"

#include "Nand/NandDevice.h"
#include "Nand/NandHal.h"

TEST(NandDeviceTest, Basic) {
	constexpr U32 blockCount = 64;
	constexpr U32 pagesPerBlock = 256;
	constexpr U32 bytesPerPage = 8192;

	NandDevice nandDevice(blockCount, pagesPerBlock, bytesPerPage);

	U8 pDataToWrite[bytesPerPage];
	for (auto i(0); i < bytesPerPage; ++i)
	{
		pDataToWrite[i] = i % 255;
	}
	U8 pDataRead[bytesPerPage];
	U8 pErasedBuffer[bytesPerPage];
	std::memset(pErasedBuffer, NandBlock::ERASED_PATTERN, bytesPerPage);

	tBlockInChip block;
	block._ = 0;
	for ( ; block._ < blockCount; ++block._)
	{
		tPageInBlock page;
		page._ = 0;
		for (; page._ < pagesPerBlock; ++page._)
		{
			nandDevice.WritePage(block, page, pDataToWrite);

			nandDevice.ReadPage(block, page, pDataRead);
			auto result = std::memcmp(pDataToWrite, pDataRead, bytesPerPage);
			ASSERT_EQ(0, result);
		}

		nandDevice.EraseBlock(block);
		for (; page._ < pagesPerBlock; ++page._)
		{
			nandDevice.ReadPage(block, page, pDataRead);
			auto result = std::memcmp(pErasedBuffer, pDataRead, bytesPerPage);
			ASSERT_EQ(0, result);
		}
	}
}

TEST(NandHalTest, Basic) {
	constexpr U8 chips = 2;
	constexpr U32 blocks = 64;
	constexpr U32 pages = 128;
	constexpr U32 bytes = 8192;

	NandHal nandHal;
	nandHal.PreInit(chips, blocks, pages, bytes);
	nandHal.Init();

	U8 pDataToWrite[bytes];
	for (auto i(0); i < bytes; ++i)
	{
		pDataToWrite[i] = i % 255;
	}
	U8 pDataRead[bytes];
	U8 pErasedBuffer[bytes];
	std::memset(pErasedBuffer, NandBlock::ERASED_PATTERN, bytes);

	tChip chip;
	chip._ = 0;
	for ( ; chip._ < chips; ++chip._)
	{
		tBlockInChip block;
		block._ = 0;
		for (; block._ < blocks; ++block._)
		{
			tPageInBlock page;
			page._ = 0;
			for (; page._ < pages; ++page._)
			{
				nandHal.WritePage(chip, block, page, pDataToWrite);
				nandHal.ReadPage(chip, block, page, pDataRead);
				auto result = std::memcmp(pDataToWrite, pDataRead, bytes);
				ASSERT_EQ(0, result);
			}

			nandHal.EraseBlock(chip, block);
			for (; page._ < pages; ++page._)
			{
				nandHal.ReadPage(chip, block, page, pDataRead);
				auto result = std::memcmp(pErasedBuffer, pDataRead, bytes);
				ASSERT_EQ(0, result);
			}
		}
	}
}