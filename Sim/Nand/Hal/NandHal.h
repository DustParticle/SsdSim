#ifndef __NandHal_h__
#define __NandHal_h__

#include <vector>
#include <queue>
#include <memory>

#include "boost/lockfree/spsc_queue.hpp"

#include "SimFrameworkBase/FrameworkThread.h"
#include "Nand/Sim/NandChannel.h"

class NandHal : public FrameworkThread
{
public:
	NandHal();

	//NOTE: With current design, we only support homogeneous NAND device configuration (i.e. all the NAND devices are the same).\
	//PreInit is for simulation system only (i.e. there would be no equipvalent on target)
	void PreInit(U8 channelCount, U8 deviceCount, U32 blocksPerDevice, U32 pagesPerBlock, U32 bytesPerPage);
	
public:
	void Init();

public:
    struct Geometry
    {
        U8 _ChannelCount;
        U8 _DevicesPerChannel;
        U32 _BlocksPerDevice;
        U32 _PagesPerBlock;
        U32 _BytesPerPage;
    };

    inline Geometry GetGeometry() const { return _Geometry; }

public:
    struct NandAddress
    {
        tChannel Channel;
        tDeviceInChannel Device;
        tBlockInDevice Block;
        tPageInBlock Page;
    };

	struct CommandDesc
	{
		enum class Op
		{
			Read,
			Write,
			Erase,
			ReadPartial,
			WritePartial,
		};

        NandAddress Address;
		Op	Operation;
		U8* Buffer;
		tByteOffset ByteOffset;
		tByteCount ByteCount;
	};

	void QueueCommand(const CommandDesc& command);

	bool IsCommandQueueEmpty() const;

public:
	void ReadPage(tChannel channel, tDeviceInChannel device, tBlockInDevice block, tPageInBlock page, U8* const pOutData);
	void ReadPage(
		const tChannel& channel,
		const tDeviceInChannel& device,
		const tBlockInDevice& block,
		const tPageInBlock& page,
		const tByteOffset& byteOffset, 
		const tByteCount& byteCount, 
		U8* const outBuffer);

	void WritePage(tChannel channel, tDeviceInChannel device, tBlockInDevice block, tPageInBlock page, const U8* const pInData);
	void WritePage(
		const tChannel& channel, 
		const tDeviceInChannel& device, 
		const tBlockInDevice& block, 
		const tPageInBlock& page, 
		const tByteOffset& byteOffset, 
		const tByteCount& byteCount, 
		const U8* const inBuffer);

	void EraseBlock(tChannel channel, tDeviceInChannel chip, tBlockInDevice block);

protected:
	virtual void Run() override;

private:
	std::vector<NandChannel> _NandChannels;

	std::unique_ptr<boost::lockfree::spsc_queue<CommandDesc>> _CommandQueue;

    Geometry _Geometry;
};

#endif
