#include "Nand/NandHal.h"

void NandHal::PreInit(U8 channelCount, U8 deviceCount, U32 blocksPerPage, U32 pagesPerBlock, U32 bytesPerPage)
{
	_ChannelCount = channelCount;
	_DeviceCount = deviceCount;
	_BlocksPerDevice = blocksPerPage;
	_PagesPerBlock = pagesPerBlock;
	_BytesPerPage = bytesPerPage;
}

void NandHal::Init()
{
	//Normally in hardware implementation we would query each device
	//Here we rely on PreInit

	for (U8 i(0); i < _ChannelCount; ++i)
	{
		NandChannel nandChannel;
		nandChannel.Init(_DeviceCount, _BlocksPerDevice, _PagesPerBlock, _BytesPerPage);
		_NandChannels.push_back(std::move(nandChannel));
	}
}

void NandHal::ReadPage(tChannel channel, tDeviceInChannel device, tBlockInDevice block, tPageInBlock page, U8* const pOutData)
{
	_NandChannels[channel._][device._].ReadPage(block, page, pOutData);
}

void NandHal::WritePage(tChannel channel, tDeviceInChannel device, tBlockInDevice block, tPageInBlock page, const U8* const pInData)
{
	_NandChannels[channel._][device._].WritePage(block, page, pInData);
}

void NandHal::EraseBlock(tChannel channel, tDeviceInChannel device, tBlockInDevice block)
{
	_NandChannels[channel._][device._].EraseBlock(block);
}

void NandHal::Run()
{
	while (false == IsStopRequested())
	{
		//Later will add processing
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}