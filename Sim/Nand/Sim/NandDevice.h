#ifndef __NandDevice_h__
#define __NandDevice_h__

#include <memory>
#include <vector>

#include "Nand/Sim/NandDeviceDesc.h"
#include "Nand/Sim/NandBlock.h"

class NandDevice
{
public:
	NandDevice(U32 blockCount, U32 pagesPerBlock, U32 bytesPerPage);
	NandDevice(NandDevice&& rhs) = default;

public:
	void ReadPage(tBlockInDevice block, tPageInBlock page, U8* const pOutData);
	void ReadPage(const tBlockInDevice& block, const tPageInBlock& page, const tByteOffset& byteOffset, const tByteCount& byteCount, U8* const outBuffer);

	void WritePage(tBlockInDevice block, tPageInBlock page, const U8* const pInData);
	void WritePage(const tBlockInDevice& block, const tPageInBlock& page, const tByteOffset& byteOffset, const tByteCount& byteCount, const U8* const inBuffer);

	void EraseBlock(tBlockInDevice block);

private:
	std::unique_ptr<NandDeviceDesc> _Desc;
	std::vector<NandBlock> _Blocks;
};

#endif