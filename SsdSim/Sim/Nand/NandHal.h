#ifndef __NandHal_h__
#define __NandHal_h__

#include <vector>

#include "Nand/NandDevice.h"

class NandHal
{
public:
	void PreInit(U8 chipCount, U32 blocksPerChip, U32 pagesPerBlock, U32 bytesPerPage);
	void Init();

public:
	void ReadPage(tChip chip, tBlockInChip block, tPageInBlock page, U8* const pOutData);

	void WritePage(tChip chip, tBlockInChip block, tPageInBlock page, const U8* const pInData);

	void EraseBlock(tChip chip, tBlockInChip block);

private:
	U8 _ChipCount;
	U32 _BlocksPerChip;
	U32 _PagesPerBlock;
	U32 _BytesPerPage;

private:
	std::vector<NandDevice> _NandDevices;
};

#endif
