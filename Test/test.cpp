#include "pch.h"

#include <future>

#include "Nand/NandDevice.h"
#include "Nand/NandHal.h"
#include "Framework.h"

TEST(NandDeviceTest, Basic) {
	constexpr U32 blockCount = 64;
	constexpr U32 pagesPerBlock = 256;
	constexpr U32 bytesPerPage = 8192;

	NandDevice nandDevice(blockCount, pagesPerBlock, bytesPerPage);

	U8 pDataToWrite[bytesPerPage];
	for (auto i(0); i < sizeof(pDataToWrite); ++i)
	{
		pDataToWrite[i] = i % 255;
	}
	U8 pDataRead[bytesPerPage];
	U8 pErasedBuffer[bytesPerPage];
	std::memset(pErasedBuffer, NandBlock::ERASED_PATTERN, sizeof(pErasedBuffer));

	tBlockInDevice block;
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
	constexpr U8 channels = 4;
	constexpr U8 devices = 2;
	constexpr U32 blocks = 64;
	constexpr U32 pages = 256;
	constexpr U32 bytes = 8192;

	NandHal nandHal;
	nandHal.PreInit(channels, devices, blocks, pages, bytes);
	nandHal.Init();

	U8 pDataToWrite[bytes];
	for (auto i(0); i < sizeof(pDataToWrite); ++i)
	{
		pDataToWrite[i] = i % 255;
	}
	U8 pDataRead[bytes];
	U8 pErasedBuffer[bytes];
	std::memset(pErasedBuffer, NandBlock::ERASED_PATTERN, sizeof(pErasedBuffer));

	tChannel channel;
	channel._ = 0;
	for (; channel._ < channels; ++channel._)
	{
		tDeviceInChannel device;
		device._ = 0;
		for (; device._ < devices; ++device._)
		{
			tBlockInDevice block;
			block._ = 0;
			for (; block._ < blocks; ++block._)
			{
				tPageInBlock page;
				page._ = 0;
				for (; page._ < pages; ++page._)
				{
					nandHal.WritePage(channel, device, block, page, pDataToWrite);

					nandHal.ReadPage(channel, device, block, page, pDataRead);
					auto result = std::memcmp(pDataToWrite, pDataRead, bytes);
					ASSERT_EQ(0, result);
				}

				nandHal.EraseBlock(channel, device, block);
				for (; page._ < pages; ++page._)
				{
					nandHal.ReadPage(channel, device, block, page, pDataRead);
					auto result = std::memcmp(pErasedBuffer, pDataRead, bytes);
					ASSERT_EQ(0, result);
				}
			}
		}
	}
}

TEST(SimFramework, Basic)
{
	Framework framework;
	auto fwFuture = std::async(std::launch::async, &(Framework::Run), &framework);

	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	framework.PushMessage(Framework::Message::Exit);
}