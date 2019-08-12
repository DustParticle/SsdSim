#include "Nand/Hal/NandHal.h"

NandHal::NandHal()
{
	_CommandQueue = std::unique_ptr<boost::lockfree::spsc_queue<CommandDesc>>(new boost::lockfree::spsc_queue<CommandDesc>{ 1024 });
	_FinishedCommandQueue = std::unique_ptr<boost::lockfree::spsc_queue<CommandDesc>>(new boost::lockfree::spsc_queue<CommandDesc>{ 1024 });
}

void NandHal::PreInit(U8 channelCount, U8 deviceCount, U32 blocksPerPage, U32 pagesPerBlock, U32 bytesPerPage)
{
	_Geometry._ChannelCount = channelCount;
	_Geometry._DevicesPerChannel = deviceCount;
	_Geometry._BlocksPerDevice = blocksPerPage;
	_Geometry._PagesPerBlock = pagesPerBlock;
	_Geometry._BytesPerPage = bytesPerPage;
}

void NandHal::Init(BufferHal *bufferHal)
{
	//Normally in hardware implementation we would query each device
	//Here we rely on PreInit

	for (U8 i(0); i < _Geometry._ChannelCount; ++i)
	{
		NandChannel nandChannel;
		nandChannel.Init(_Geometry._DevicesPerChannel, _Geometry._BlocksPerDevice, _Geometry._PagesPerBlock, _Geometry._BytesPerPage);
		_NandChannels.push_back(std::move(nandChannel));
	}

    _BufferHal = bufferHal;
}

void NandHal::QueueCommand(const CommandDesc& command)
{
	_CommandQueue->push(command);
}

bool NandHal::IsCommandQueueEmpty() const
{
	return _CommandQueue->empty();
}

bool NandHal::PopFinishedCommand(CommandDesc& command)
{
	return (_FinishedCommandQueue->pop(command));
}

bool NandHal::ReadPage(tChannel channel, tDeviceInChannel device, tBlockInDevice block, tPageInBlock page, const Buffer &outBuffer)
{
	return (_NandChannels[channel._][device._].ReadPage(block, page, _BufferHal->ToPointer(outBuffer)));
}

bool NandHal::ReadPage(const tChannel& channel,
	const tDeviceInChannel& device,
	const tBlockInDevice& block,
	const tPageInBlock& page,
	const tByteOffset& byteOffset,
	const tByteCount& byteCount,
    const Buffer &outBuffer)
{
	return (_NandChannels[channel._][device._].ReadPage(block, page, byteOffset, byteCount, _BufferHal->ToPointer(outBuffer)));
}

void NandHal::WritePage(tChannel channel, tDeviceInChannel device, tBlockInDevice block, tPageInBlock page, const Buffer &inBuffer)
{
	_NandChannels[channel._][device._].WritePage(block, page, _BufferHal->ToPointer(inBuffer));
}

void NandHal::WritePage(const tChannel& channel,
	const tDeviceInChannel& device,
	const tBlockInDevice& block,
	const tPageInBlock& page,
	const tByteOffset& byteOffset,
	const tByteCount& byteCount,
    const Buffer &inBuffer)
{
	_NandChannels[channel._][device._].WritePage(block, page, byteOffset, byteCount, _BufferHal->ToPointer(inBuffer));
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

		// Set defaut return status is Success
		command.CommandStatus = CommandDesc::Status::Success;

		switch (command.Operation)
		{
			case CommandDesc::Op::Read:
			{
				if (false == ReadPage(address.Channel, address.Device, address.Block, address.Page, command.Buffer))
				{
					command.CommandStatus = CommandDesc::Status::Uecc;
				}
			}break;
			case CommandDesc::Op::Write:
			{
				// TODO: Update command status
				WritePage(address.Channel, address.Device, address.Block, address.Page, command.Buffer);
			}break;
			case CommandDesc::Op::Erase:
			{
				// TODO: Update command status
				EraseBlock(address.Channel, address.Device, address.Block);
			}break;
			case CommandDesc::Op::ReadPartial:
			{
				if (false == ReadPage(address.Channel, address.Device, address.Block, address.Page, command.ByteOffset, command.ByteCount, command.Buffer))
				{
					command.CommandStatus = CommandDesc::Status::Uecc;
				}
			}break;
			case CommandDesc::Op::WritePartial:
			{
				// TODO: Update command status
				WritePage(address.Channel, address.Device, address.Block, address.Page, command.ByteOffset, command.ByteCount, command.Buffer);
			}break;
		}

		_FinishedCommandQueue->push(command);
		_CommandQueue->pop();
	}
}
