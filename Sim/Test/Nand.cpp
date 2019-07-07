#include "pch.h"

#include <array>

#include "Buffer/Hal/BufferHal.h"
#include "Nand/Sim/NandDevice.h"
#include "Nand/Hal/NandHal.h"

TEST(NandSim, Basic)
{
	constexpr U32 blockCount = 64;
	constexpr U32 pagesPerBlock = 256;
	constexpr U32 bytesPerPage = 8192;

	NandDevice nandDevice(blockCount, pagesPerBlock, bytesPerPage);

	U8 pWriteBuffer[bytesPerPage];
	for (auto i(0); i < sizeof(pWriteBuffer); ++i)
	{
        pWriteBuffer[i] = i % 255;
	}
	U8 pReadBuffer[bytesPerPage];
	U8 pErasedBuffer[bytesPerPage];
	std::memset(pErasedBuffer, NandBlock::ERASED_PATTERN, sizeof(pErasedBuffer));

	tBlockInDevice block;
	block._ = 0;
	for (; block._ < blockCount; ++block._)
	{
		tPageInBlock page;
		page._ = 0;
		for (; page._ < pagesPerBlock; ++page._)
		{
			nandDevice.WritePage(block, page, pWriteBuffer);

			nandDevice.ReadPage(block, page, pReadBuffer);
			auto result = std::memcmp(pWriteBuffer, pReadBuffer, bytesPerPage);
			ASSERT_EQ(0, result);
		}

		nandDevice.EraseBlock(block);
		for (; page._ < pagesPerBlock; ++page._)
		{
			nandDevice.ReadPage(block, page, pReadBuffer);
			auto result = std::memcmp(pErasedBuffer, pReadBuffer, bytesPerPage);
			ASSERT_EQ(0, result);
		}
	}
}

TEST(NandHal, Basic) {
	constexpr U8 channels = 4;
	constexpr U8 devices = 2;
	constexpr U32 blocks = 64;
	constexpr U32 pages = 256;
	constexpr U32 bytes = 8192;
    constexpr U32 sectorsPerPage = bytes / 512;
    constexpr U32 maxBufferSizeInKB = sectorsPerPage;

    BufferHal bufferHal;
    bufferHal.PreInit(maxBufferSizeInKB);

	NandHal nandHal;
	nandHal.PreInit(channels, devices, blocks, pages, bytes);
	nandHal.Init(&bufferHal);

    Buffer writeBuffer, readBuffer;
    ASSERT_TRUE(bufferHal.AllocateBuffer(sectorsPerPage, writeBuffer));
	U8 *pWriteData = bufferHal.ToPointer(writeBuffer);
	for (auto i(0); i < sizeof(pWriteData); ++i)
	{
        pWriteData[i] = i % 255;
	}
    ASSERT_TRUE(bufferHal.AllocateBuffer(sectorsPerPage, readBuffer));
    U8 *pReadData = bufferHal.ToPointer(readBuffer);
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
					nandHal.WritePage(channel, device, block, page, writeBuffer);

					nandHal.ReadPage(channel, device, block, page, readBuffer);
					auto result = std::memcmp(pWriteData, pReadData, bytes);
					ASSERT_EQ(0, result);
				}

				nandHal.EraseBlock(channel, device, block);
				for (; page._ < pages; ++page._)
				{
					nandHal.ReadPage(channel, device, block, page, readBuffer);
					auto result = std::memcmp(pErasedBuffer, pReadData, bytes);
					ASSERT_EQ(0, result);
				}
			}
		}
	}

    bufferHal.DeallocateBuffer(writeBuffer);
    bufferHal.DeallocateBuffer(readBuffer);
}

TEST(NandHal, Basic_CommandQueue)
{
	constexpr U8 channels = 4;
	constexpr U8 devices = 2;
	constexpr U32 blocks = 64;
	constexpr U32 pages = 256;
	constexpr U32 bytes = 8192;
	constexpr U8 bufferCount = channels * 4;
	constexpr U32 commandCount = channels * devices * blocks * pages;
    constexpr U32 sectorsPerPage = bytes / 512;
    constexpr U32 maxBufferSizeInKB = sectorsPerPage * bufferCount;

    BufferHal bufferHal;
    bufferHal.PreInit(maxBufferSizeInKB);

	std::array<Buffer, bufferCount> writeBuffers;
    std::array<Buffer, bufferCount> readBuffers;

    for (U32 i(0); i < bufferCount; ++i)
	{
        ASSERT_TRUE(bufferHal.AllocateBuffer(sectorsPerPage, writeBuffers[i]));
        ASSERT_TRUE(bufferHal.AllocateBuffer(sectorsPerPage, readBuffers[i]));

        U8* pBuffer = bufferHal.ToPointer(writeBuffers[i]);
		for (auto i(0); i < bytes; ++i)
		{
			pBuffer[i] = i % 255;
		}
	}

	U8 pErasedBuffer[bytes];
	std::memset(pErasedBuffer, NandBlock::ERASED_PATTERN, sizeof(pErasedBuffer));

	NandHal nandHal;
	nandHal.PreInit(channels, devices, blocks, pages, bytes);
	nandHal.Init(&bufferHal);

	std::future<void> nandHalFuture;

	nandHalFuture = std::async(std::launch::async, &NandHal::operator(), &nandHal);

	NandHal::CommandDesc commandDesc;
    NandHal::NandAddress& address = commandDesc.Address;
	address.Channel._ = 0;
	address.Device._ = 0;
	address.Block._ = 0;
	address.Page._ = 0;
	for (auto c(0); c < commandCount;)
	{
		for (auto i(0); i < bufferCount; ++i, ++c)
		{
			commandDesc.Operation = NandHal::CommandDesc::Op::Write;
			commandDesc.Buffer = writeBuffers[i];
			nandHal.QueueCommand(commandDesc);

			commandDesc.Operation = NandHal::CommandDesc::Op::Read;
			commandDesc.Buffer = readBuffers[i];
			nandHal.QueueCommand(commandDesc);

			if (++address.Channel._ >= channels)
			{
                address.Channel._ = 0;
				if (++address.Device._ >= devices)
				{
                    address.Device._ = 0;
					if (++address.Page._ >= pages)
					{
                        address.Page._ = 0;
						ASSERT_TRUE(address.Block._ < blocks);	//let's not go pass this boundary
						++address.Block._;
					}
				}
			}
		}

		while (false == nandHal.IsCommandQueueEmpty());
	}

	nandHal.Stop();

    for (U32 i(0); i < bufferCount; ++i)
    {
        bufferHal.DeallocateBuffer(writeBuffers[i]);
        bufferHal.DeallocateBuffer(readBuffers[i]);
    }
}