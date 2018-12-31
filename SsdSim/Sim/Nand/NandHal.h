#ifndef __NandHal_h__
#define __NandHal_h__

#include <vector>

#include "FrameworkThread.h"
#include "Nand/NandChannel.h"

class NandHal : public FrameworkThread
{
public:
	//NOTE: With current design, we only support homogeneous NAND device configuration (i.e. all the NAND devices are the same).
	void PreInit(U8 channelCount, U8 deviceCount, U32 blocksPerDevice, U32 pagesPerBlock, U32 bytesPerPage);
	void Init();

public:
	void ReadPage(tChannel channel, tDeviceInChannel device, tBlockInDevice block, tPageInBlock page, U8* const pOutData);

	void WritePage(tChannel channel, tDeviceInChannel chip, tBlockInDevice block, tPageInBlock page, const U8* const pInData);

	void EraseBlock(tChannel channel, tDeviceInChannel chip, tBlockInDevice block);

protected:
	virtual void Run() override;

private:
	U8 _ChannelCount;
	U8 _DeviceCount;
	U32 _BlocksPerDevice;
	U32 _PagesPerBlock;
	U32 _BytesPerPage;

private:
	std::vector<NandChannel> _NandChannels;
};

#endif
