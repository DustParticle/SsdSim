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
    for (decltype(writeBuffer.SizeInByte) i{ 0 }; i < writeBuffer.SizeInByte; ++i)
    {
        pWriteBuffer[i] = i % 255;
    }

    Buffer readBuffer;
    _BufferHal->AllocateBuffer(BufferType::System, sectorsPerPage, readBuffer);
    U8 *pReadBuffer = _BufferHal->ToPointer(readBuffer);
    U8 pErasedBuffer[bytesPerPage];
    std::memset(pErasedBuffer, NandBlock::ERASED_PATTERN, sizeof(pErasedBuffer));

    for (tBlockInDevice block{ 0 }; block < blockCount; ++block)
    {
        for (tPageInBlock page{ 0 }; page < pagesPerBlock; ++page)
        {
            _NandDevice->WritePage(block, page, writeBuffer);

            _NandDevice->ReadPage(block, page, readBuffer);
            auto result = std::memcmp(pWriteBuffer, pReadBuffer, bytesPerPage);
            ASSERT_EQ(0, result);
        }

        _NandDevice->EraseBlock(block);

        for (tPageInBlock page{ 0 }; page < pagesPerBlock; ++page)
        {
            _NandDevice->ReadPage(block, page, readBuffer);
            auto result = std::memcmp(pErasedBuffer, pReadBuffer, bytesPerPage);
            ASSERT_EQ(0, result);
        }
    }

    _BufferHal->DeallocateBuffer(writeBuffer);
    _BufferHal->DeallocateBuffer(readBuffer);
}

class NandHalTest : public ::testing::Test, public NandHal::CommandListener
{
public:
    U32 _CompletedNandCount;

    virtual void HandleCommandCompleted(const NandHal::CommandDesc &command)
    {
        ++_CompletedNandCount;
    }

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
        _CompletedNandCount = 0;
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

    for (tChannel channel{ 0 }; channel < channels; ++channel)
    {
        for (tDeviceInChannel device{ 0 }; device < devices; ++device)
        {
            for (tBlockInDevice block{ 0 }; block < blocks; ++block)
            {
                for (tPageInBlock page{ 0 }; page < pages; ++page)
                {
                    _NandHal->WritePage(channel, device, block, page, writeBuffer);

                    _NandHal->ReadPage(channel, device, block, page, readBuffer);
                    auto result = std::memcmp(pWriteData, pReadData, bytes);
                    ASSERT_EQ(0, result);
                }

                _NandHal->EraseBlock(channel, device, block);

                for (tPageInBlock page{ 0 }; page < pages; ++page)
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

    U32 queuedCommand = 0;
	NandHal::CommandDesc commandDesc;
    NandHal::NandAddress& address = commandDesc.Address;
	address.Channel = 0;
	address.Device = 0;
	address.Block = 0;
	address.Page = 0;
	for (auto c(0); c < commandCount;)
	{
		for (auto i(0); i < bufferCount; ++i, ++c)
		{
			commandDesc.Operation = NandHal::CommandDesc::Op::Write;
			commandDesc.Buffer = writeBuffers[i];
            commandDesc.Listener = this;
			_NandHal->QueueCommand(commandDesc);
            ++queuedCommand;

			commandDesc.Operation = NandHal::CommandDesc::Op::Read;
			commandDesc.Buffer = readBuffers[i];
            commandDesc.Listener = this;
            _NandHal->QueueCommand(commandDesc);
            ++queuedCommand;

			if (++address.Channel >= channels)
			{
                address.Channel = 0;
				if (++address.Device >= devices)
				{
                    address.Device = 0;
					if (++address.Page >= pages)
					{
                        address.Page = 0;
						ASSERT_TRUE(address.Block < blocks);	//let's not go pass this boundary
						++address.Block;
					}
				}
			}
		}

		while (false == _NandHal->IsCommandQueueEmpty());
	}

    ASSERT_EQ(queuedCommand, _CompletedNandCount);
	_NandHal->Stop();
    nandHalFuture.wait();

    for (U32 i{ 0 }; i < bufferCount; ++i)
    {
        _BufferHal->DeallocateBuffer(writeBuffers[i]);
        _BufferHal->DeallocateBuffer(readBuffers[i]);
    }
}