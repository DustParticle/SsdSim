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
    struct Geometry
    {
        U8 ChannelCount;
        U8 DevicesPerChannel;
        U32 BlocksPerDevice;
        U32 PagesPerBlock;
        U32 BytesPerPage;
    };

public:
	NandHal();

    //NOTE: With current design, we only support homogeneous NAND device configuration (i.e. all the NAND devices are the same).\
	//PreInit is for simulation system only (i.e. there would be no equipvalent on target)
	void PreInit(const Geometry &geometry, std::shared_ptr<BufferHal> bufferHal);
	
public:
	void Init();
    void SetSectorInfo(const SectorInfo &sectorInfo);

public:
    inline Geometry GetGeometry() const { return _Geometry; }

public:
    struct NandAddress
    {
        tChannel Channel;
        tDeviceInChannel Device;
        tBlockInDevice Block;
        tPageInBlock Page;
        tSectorInPage Sector;
        tSectorCount SectorCount;
    };

    struct CommandDesc;
    class CommandListener
    {
    public:
        virtual void HandleCommandCompleted(const CommandDesc &command) = 0;
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

        U32 DescSectorIndex;
        CommandListener *Listener;
	};

	void QueueCommand(const CommandDesc& command);
	bool IsCommandQueueEmpty() const;

public:
	bool ReadPage(tChannel channel, tDeviceInChannel device, tBlockInDevice block, tPageInBlock page, const Buffer &outBuffer);
	bool ReadPage(
		const tChannel& channel,
		const tDeviceInChannel& device,
		const tBlockInDevice& block,
		const tPageInBlock& page,
		const tSectorInPage& sector, 
		const tSectorCount& sectorCount,
        const Buffer &outBuffer);

	void WritePage(tChannel channel, tDeviceInChannel device, tBlockInDevice block, tPageInBlock page, const Buffer &inBuffer);
	void WritePage(
		const tChannel& channel, 
		const tDeviceInChannel& device, 
		const tBlockInDevice& block, 
		const tPageInBlock& page, 
        const tSectorInPage& sector,
        const tSectorCount& sectorCount,
        const Buffer &inBuffer);

	void EraseBlock(tChannel channel, tDeviceInChannel chip, tBlockInDevice block);

protected:
	virtual void Run() override;

private:
    std::shared_ptr<BufferHal> _BufferHal;

	std::vector<NandChannel> _NandChannels;

	std::unique_ptr<boost::lockfree::spsc_queue<CommandDesc>> _CommandQueue;

    Geometry _Geometry;
    SectorInfo _SectorInfo;
};

#endif
