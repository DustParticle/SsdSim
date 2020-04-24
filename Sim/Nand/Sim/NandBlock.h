#ifndef __NandBlock_h__
#define __NandBlock_h__

#include <memory>

#include "Nand/Types.h"
#include "Nand/Sim/NandBlockTracker.h"
#include "Nand/Sim/NandDeviceDesc.h"

#include "Buffer/Types.h"
#include "Buffer/Hal/BufferHal.h"

class NandBlock
{
public:
	NandBlock(BufferHal *bufferHal, U32 pagesPerBlock, U32 totalBytesPerPage);
	NandBlock(NandBlock&& rhs) = default;

public:
	void Erase();

	void WritePage(tPageInBlock page, const Buffer &inBuffer);
	void WritePage(const tPageInBlock& page, const tSectorInPage& sector, const tSectorCount& sectorCount, const Buffer &inBuffer, const tSectorOffset& bufferOffset);

	bool ReadPage(tPageInBlock page, const Buffer &outBuffer);
	bool ReadPage(const tPageInBlock& page, const tSectorInPage& sector, const tSectorCount& sectorCount, const Buffer &outBuffer, const tSectorOffset& bufferOffset);

public:
	static const U8 ERASED_PATTERN = 0xff;

private:
	NandBlockTracker _NandBlockTracker;

    BufferHal *_BufferHal;
	U32 _PagesPerBlock;
	U32 _TotalBytesPerPage;

	std::unique_ptr<U8[]> _Buffer;
	std::unique_ptr<U8[]> _ErasedBuffer;
};

#endif
