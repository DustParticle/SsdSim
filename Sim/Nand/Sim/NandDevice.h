#ifndef __NandDevice_h__
#define __NandDevice_h__

#include <memory>
#include <vector>

#include "Nand/Sim/NandDeviceDesc.h"
#include "Nand/Sim/NandBlock.h"

#include "Buffer/Types.h"
#include "Buffer/Hal/BufferHal.h"

class NandDevice
{
public:
	NandDevice(BufferHal *bufferHal, U32 blockCount, U32 pagesPerBlock, U32 bytesPerPage);
	NandDevice(NandDevice&& rhs) = default;

public:
	bool ReadPage(tBlockInDevice block, tPageInBlock page, const Buffer &outBuffer);
	bool ReadPage(const tBlockInDevice& block, const tPageInBlock& page, const tSectorInPage& sector, const tSectorCount& sectorCount, 
        const Buffer &outBuffer, const tSectorOffset& bufferOffset);

	void WritePage(tBlockInDevice block, tPageInBlock page, const Buffer &inBuffer);
	void WritePage(const tBlockInDevice& block, const tPageInBlock& page, const tSectorInPage& sector, const tSectorCount& sectorCount, 
        const Buffer &inBuffer, const tSectorOffset& bufferOffset);

	void EraseBlock(tBlockInDevice block);

private:
	std::unique_ptr<NandDeviceDesc> _Desc;
	std::vector<NandBlock> _Blocks;
};

#endif