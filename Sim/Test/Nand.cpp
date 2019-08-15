#include "pch.h"

#include <array>

#include "Buffer/Hal/BufferHal.h"
#include "Nand/Sim/NandDevice.h"
#include "Nand/Hal/NandHal.h"

class NandDeviceTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        _BufferHal = std::make_shared<BufferHal>();
        _BufferHal->PreInit(maxBufferSizeInKB);

        _NandDevice = std::make_unique<NandDevice>(_BufferHal.get(), blockCount, pagesPerBlock, bytesPerPage);
    }

    void TearDown() override
    {
    }

    static const U32 blockCount = 64;
    static const U32 pagesPerBlock = 256;
    static const U32 bytesPerPage = 8192;
    static const U32 sectorsPerPage = bytesPerPage / 512;
    static const U32 maxBufferSizeInKB = sectorsPerPage;

    std::shared_ptr<BufferHal> _BufferHal;
    std::unique_ptr<NandDevice> _NandDevice;
};

TEST_F(NandDeviceTest, Basic)
{
    Buffer writeBuffer;
    _BufferHal->AllocateBuffer(BufferType::System, sectorsPerPage, writeBuffer);
    U8 *pWriteBuffer = _BufferHal->ToPointer(writeBuffer);
    for (auto i(0); i < writeBuffer.SizeInByte; ++i)
    {
        pWriteBuffer[i] = i % 255;
    }

    Buffer readBuffer;
    _BufferHal->AllocateBuffer(BufferType::System, sectorsPerPage, readBuffer);
    U8 *pReadBuffer = _BufferHal->ToPointer(readBuffer);
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
            _NandDevice->WritePage(block, page, writeBuffer);

            _NandDevice->ReadPage(block, page, readBuffer);
            auto result = std::memcmp(pWriteBuffer, pReadBuffer, bytesPerPage);
            ASSERT_EQ(0, result);
        }

        _NandDevice->EraseBlock(block);
        for (; page._ < pagesPerBlock; ++page._)
        {
            _NandDevice->ReadPage(block, page, readBuffer);
            auto result = std::memcmp(pErasedBuffer, pReadBuffer, bytesPerPage);
            ASSERT_EQ(0, result);
        }
    }

    _BufferHal->DeallocateBuffer(writeBuffer);
    _BufferHal->DeallocateBuffer(readBuffer);
}

class NandHalTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        _BufferHal = std::make_shared<BufferHal>();
        _BufferHal->PreInit(maxBufferSizeInKB);

        NandHal::Geometry geometry;
        geometry.ChannelCount = channels;
        geometry.DevicesPerChannel = devices;
        geometry.BlocksPerDevice = blocks;
        geometry.PagesPerBlock = pages;
        geometry.BytesPerPage = bytes;

        _NandHal = std::make_shared<NandHal>();
        _NandHal->PreInit(geometry, _BufferHal);
        _NandHal->Init();
    }

    void TearDown() override
    {
    }

    static const U8 channels = 4;
    static const U8 devices = 2;
    static const U32 blocks = 64;
    static const U32 pages = 256;
    static const U32 bytes = 8192;
    static const U32 sectorsPerPage = bytes / 512;
    static const U32 maxBufferSizeInKB = 1024;

    std::shared_ptr<BufferHal> _BufferHal;
    std::shared_ptr<NandHal> _NandHal;
};


TEST_F(NandHalTest, Basic)
{
    Buffer writeBuffer, readBuffer;
    ASSERT_TRUE(_BufferHal->AllocateBuffer(BufferType::User, sectorsPerPage, writeBuffer));
    U8 *pWriteData = _BufferHal->ToPointer(writeBuffer);
    for (auto i(0); i < sizeof(pWriteData); ++i)
    {
        pWriteData[i] = i % 255;
    }
    ASSERT_TRUE(_BufferHal->AllocateBuffer(BufferType::User, sectorsPerPage, readBuffer));
    U8 *pReadData = _BufferHal->ToPointer(readBuffer);
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
                    _NandHal->WritePage(channel, device, block, page, writeBuffer);

                    _NandHal->ReadPage(channel, device, block, page, readBuffer);
                    auto result = std::memcmp(pWriteData, pReadData, bytes);
                    ASSERT_EQ(0, result);
                }

                _NandHal->EraseBlock(channel, device, block);
                for (; page._ < pages; ++page._)
                {
                    _NandHal->ReadPage(channel, device, block, page, readBuffer);
                    auto result = std::memcmp(pErasedBuffer, pReadData, bytes);
                    ASSERT_EQ(0, result);
                }
            }
        }
    }

    _BufferHal->DeallocateBuffer(writeBuffer);
    _BufferHal->DeallocateBuffer(readBuffer);
}

TEST_F(NandHalTest, Basic_CommandQueue)
{
	constexpr U8 bufferCount = channels * 4;
	constexpr U32 commandCount = channels * devices * blocks * pages;

	std::array<Buffer, bufferCount> writeBuffers;
    std::array<Buffer, bufferCount> readBuffers;

    for (U32 i(0); i < bufferCount; ++i)
	{
        ASSERT_TRUE(_BufferHal->AllocateBuffer(BufferType::User, sectorsPerPage, writeBuffers[i]));
        ASSERT_TRUE(_BufferHal->AllocateBuffer(BufferType::User, sectorsPerPage, readBuffers[i]));

        U8* pBuffer = _BufferHal->ToPointer(writeBuffers[i]);
		for (auto i(0); i < bytes; ++i)
		{
			pBuffer[i] = i % 255;
		}
	}

	U8 pErasedBuffer[bytes];
	std::memset(pErasedBuffer, NandBlock::ERASED_PATTERN, sizeof(pErasedBuffer));

	std::future<void> nandHalFuture;

	nandHalFuture = std::async(std::launch::async, &NandHal::operator(), _NandHal);

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
			_NandHal->QueueCommand(commandDesc);

			commandDesc.Operation = NandHal::CommandDesc::Op::Read;
			commandDesc.Buffer = readBuffers[i];
			_NandHal->QueueCommand(commandDesc);

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

		while (false == _NandHal->IsCommandQueueEmpty());
	}

	_NandHal->Stop();
    nandHalFuture.wait();

    for (U32 i(0); i < bufferCount; ++i)
    {
        _BufferHal->DeallocateBuffer(writeBuffers[i]);
        _BufferHal->DeallocateBuffer(readBuffers[i]);
    }
}