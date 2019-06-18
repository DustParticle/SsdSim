#ifndef __NandHal_h__
#define __NandHal_h__

#include <vector>
#include <queue>
#include <memory>

#include "boost/lockfree/spsc_queue.hpp"

#include "SimFrameworkBase/FrameworkThread.h"
#include "Nand/Sim/NandChannel.h"
#include "Buffer/Hal/BufferHal.h"

class NandHal : public FrameworkThread
{
public:
	NandHal();

	//NOTE: With current design, we only support homogeneous NAND device configuration (i.e. all the NAND devices are the same).\
	//PreInit is for simulation system only (i.e. there would be no equipvalent on target)
	void PreInit(U8 channelCount, U8 deviceCount, U32 blocksPerDevice, U32 pagesPerBlock, U32 bytesPerPage);
	
public:
	void Init(std::shared_ptr<BufferHal> bufferHal);

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

		enum class Status
		{
			Success,
			Uecc,
			WriteError,
			EraseError,
		};

		NandAddress Address;
		Op Operation;
		Status CommandStatus;
		Buffer Buffer;
		tByteOffset ByteOffset;
		tByteCount ByteCount;

        U8 *DescBuffer;
	};

	void QueueCommand(const CommandDesc& command);
	bool IsCommandQueueEmpty() const;
	bool PopFinishedCommand(CommandDesc& command);

public:
	bool ReadPage(tChannel channel, tDeviceInChannel device, tBlockInDevice block, tPageInBlock page, const Buffer &outBuffer);
	bool ReadPage(
		const tChannel& channel,
		const tDeviceInChannel& device,
		const tBlockInDevice& block,
		const tPageInBlock& page,
		const tByteOffset& byteOffset, 
		const tByteCount& byteCount, 
        const Buffer &outBuffer);

	void WritePage(tChannel channel, tDeviceInChannel device, tBlockInDevice block, tPageInBlock page, const Buffer &inBuffer);
	void WritePage(
		const tChannel& channel, 
		const tDeviceInChannel& device, 
		const tBlockInDevice& block, 
		const tPageInBlock& page, 
		const tByteOffset& byteOffset, 
		const tByteCount& byteCount, 
        const Buffer &inBuffer);

	void EraseBlock(tChannel channel, tDeviceInChannel chip, tBlockInDevice block);

protected:
	virtual void Run() override;

private:
	std::vector<NandChannel> _NandChannels;

    std::shared_ptr<BufferHal> _BufferHal;

	std::unique_ptr<boost::lockfree::spsc_queue<CommandDesc>> _CommandQueue;
	std::unique_ptr<boost::lockfree::spsc_queue<CommandDesc>> _FinishedCommandQueue;

    Geometry _Geometry;
};

#endif
