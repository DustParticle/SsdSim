#ifndef __NandHal_h__
#define __NandHal_h__

#include <vector>
#include <queue>
#include <memory>

#include "boost/lockfree/spsc_queue.hpp"

#include "FrameworkThread.h"
#include "Nand/NandChannel.h"

class NandHal : public FrameworkThread
{
public:
	NandHal();

	//NOTE: With current design, we only support homogeneous NAND device configuration (i.e. all the NAND devices are the same).\
	//PreInit is for simulation system only (i.e. there would be no equipvalent on target)
	void PreInit(U8 channelCount, U8 deviceCount, U32 blocksPerDevice, U32 pagesPerBlock, U32 bytesPerPage);
	void PreInit(const std::string& configFile);
	
public:
	void Init();

public:
	struct CommandDesc
	{
		enum class Op
		{
			READ,
			WRITE,
			ERASE
		};

		Op	Operation;
		tChannel Channel;
		tDeviceInChannel Device;
		tBlockInDevice Block;
		tPageInBlock Page;
		U8* Buffer;
	};

	void QueueCommand(const CommandDesc& command);

	bool IsCommandQueueEmpty() const;

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

	std::unique_ptr<boost::lockfree::spsc_queue<CommandDesc>> _CommandQueue;
};

#endif
