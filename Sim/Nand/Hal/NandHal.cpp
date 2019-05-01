#include "Nand/Hal/NandHal.h"

NandHal::NandHal()
{
	_CommandQueue = std::unique_ptr<boost::lockfree::spsc_queue<CommandDesc>>(new boost::lockfree::spsc_queue<CommandDesc>{ 1024 });
}

void NandHal::PreInit(U8 channelCount, U8 deviceCount, U32 blocksPerPage, U32 pagesPerBlock, U32 bytesPerPage)
{
	_Geometry._ChannelCount = channelCount;
	_Geometry._DevicesPerChannel = deviceCount;
	_Geometry._BlocksPerDevice = blocksPerPage;
	_Geometry._PagesPerBlock = pagesPerBlock;
	_Geometry._BytesPerPage = bytesPerPage;
}

void NandHal::Init()
{
	//Normally in hardware implementation we would query each device
	//Here we rely on PreInit

	for (U8 i(0); i < _Geometry._ChannelCount; ++i)
	{
		NandChannel nandChannel;
		nandChannel.Init(_Geometry._DevicesPerChannel, _Geometry._BlocksPerDevice, _Geometry._PagesPerBlock, _Geometry._BytesPerPage);
		_NandChannels.push_back(std::move(nandChannel));
	}
}

void NandHal::QueueCommand(const CommandDesc& command)
{
	_CommandQueue->push(command);
}

bool NandHal::IsCommandQueueEmpty() const
{
	return _CommandQueue->empty();
}

void NandHal::ReadPage(tChannel channel, tDeviceInChannel device, tBlockInDevice block, tPageInBlock page, U8* const pOutData)
{
	_NandChannels[channel._][device._].ReadPage(block, page, pOutData);
}

void NandHal::ReadPage(const tChannel& channel,
	const tDeviceInChannel& device,
	const tBlockInDevice& block,
	const tPageInBlock& page,
	const tByteOffset& byteOffset,
	const tByteCount& byteCount,
	U8* const outBuffer)
{
	_NandChannels[channel._][device._].ReadPage(block, page, byteOffset, byteCount, outBuffer);
}

void NandHal::WritePage(tChannel channel, tDeviceInChannel device, tBlockInDevice block, tPageInBlock page, const U8* const pInData)
{
	_NandChannels[channel._][device._].WritePage(block, page, pInData);
}

void NandHal::WritePage(const tChannel& channel,
	const tDeviceInChannel& device,
	const tBlockInDevice& block,
	const tPageInBlock& page,
	const tByteOffset& byteOffset,
	const tByteCount& byteCount,
	const U8* const inBuffer)
{
	_NandChannels[channel._][device._].WritePage(block, page, byteOffset, byteCount, inBuffer);
}

void NandHal::EraseBlock(tChannel channel, tDeviceInChannel device, tBlockInDevice block)
{
	_NandChannels[channel._][device._].EraseBlock(block);
}

void NandHal::Run()
{
	if (_CommandQueue->empty() == false)
	{
		CommandDesc& command = _CommandQueue->front();
        NandAddress& address = command.Address;
		switch (command.Operation)
		{
			case CommandDesc::Op::Read:
			{
				ReadPage(address.Channel, address.Device, address.Block, address.Page, command.Buffer);
			}break;
			case CommandDesc::Op::Write:
			{
				WritePage(address.Channel, address.Device, address.Block, address.Page, command.Buffer);
			}break;
			case CommandDesc::Op::Erase:
			{
				EraseBlock(address.Channel, address.Device, address.Block);
			}break;
			case CommandDesc::Op::ReadPartial:
			{
				ReadPage(address.Channel, address.Device, address.Block, address.Page, command.ByteOffset, command.ByteCount, command.Buffer);
			}break;
			case CommandDesc::Op::WritePartial:
			{
				WritePage(address.Channel, address.Device, address.Block, address.Page, command.ByteOffset, command.ByteCount, command.Buffer);
			}break;
		}
		_CommandQueue->pop();
	}
	else
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}