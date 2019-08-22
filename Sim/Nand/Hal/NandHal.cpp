#include "Nand/Hal/NandHal.h"

NandHal::NandHal()
{
	_CommandQueue = std::unique_ptr<boost::lockfree::spsc_queue<CommandDesc>>(new boost::lockfree::spsc_queue<CommandDesc>{ 1024 });
	_FinishedCommandQueue = std::unique_ptr<boost::lockfree::spsc_queue<CommandDesc>>(new boost::lockfree::spsc_queue<CommandDesc>{ 1024 });
}

void NandHal::PreInit(const Geometry &geometry, std::shared_ptr<BufferHal> bufferHal)
{
    _Geometry = geometry;
    _BufferHal = bufferHal;
}

void NandHal::Init()
{
	//Normally in hardware implementation we would query each device
	//Here we rely on PreInit

	for (U8 i(0); i < _Geometry.ChannelCount; ++i)
	{
		NandChannel nandChannel;
		nandChannel.Init(_BufferHal.get(), _Geometry.DevicesPerChannel, _Geometry.BlocksPerDevice, _Geometry.PagesPerBlock, _Geometry.BytesPerPage);
		_NandChannels.push_back(std::move(nandChannel));
	}
}

void NandHal::SetSectorInfo(const SectorInfo &sectorInfo)
{
    _SectorInfo = sectorInfo;
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
	return (_NandChannels[channel._][device._].ReadPage(block, page, outBuffer));
}

bool NandHal::ReadPage(const tChannel& channel,
	const tDeviceInChannel& device,
	const tBlockInDevice& block,
	const tPageInBlock& page,
    const tSectorInPage& sector,
    const tSectorCount& sectorCount,
    const Buffer &outBuffer)
{
	return (_NandChannels[channel._][device._].ReadPage(block, page, sector, sectorCount, outBuffer));
}

void NandHal::WritePage(tChannel channel, tDeviceInChannel device, tBlockInDevice block, tPageInBlock page, const Buffer &inBuffer)
{
	_NandChannels[channel._][device._].WritePage(block, page, inBuffer);
}

void NandHal::WritePage(const tChannel& channel,
	const tDeviceInChannel& device,
	const tBlockInDevice& block,
	const tPageInBlock& page,
    const tSectorInPage& sector,
    const tSectorCount& sectorCount,
    const Buffer &inBuffer)
{
	_NandChannels[channel._][device._].WritePage(block, page, sector, sectorCount, inBuffer);
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
				if (false == ReadPage(address.Channel, address.Device, address.Block, address.Page, address.Sector, address.SectorCount, command.Buffer))
				{
					command.CommandStatus = CommandDesc::Status::Uecc;
				}
			}break;
			case CommandDesc::Op::WritePartial:
			{
				// TODO: Update command status
				WritePage(address.Channel, address.Device, address.Block, address.Page, address.Sector, address.SectorCount, command.Buffer);
			}break;
		}

		_FinishedCommandQueue->push(command);
		_CommandQueue->pop();
	}
}
