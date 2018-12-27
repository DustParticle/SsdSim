#ifndef __NandBlock_h__
#define __NandBlock_h__

#include <memory>

#include "Common/Types.h"
#include "Nand/NandDeviceDesc.h"

class NandBlock
{
public:
	NandBlock(U32 pagesPerBlock, U32 totalBytesPerPage);
	NandBlock(NandBlock&& rhs) = default;

public:
	void Erase();

	void WritePage(tPageInBlock page, const U8* const pInData);

	void ReadPage(tPageInBlock page, U8* const pOutData);

public:
	static const U8 ERASED_PATTERN = 0xff;

private:
	U32 _PagesPerBlock;
	U32 _TotalBytesPerPage;

	std::unique_ptr<U8[]> _Buffer;
	std::unique_ptr<U8[]> _ErasedBuffer;
};

#endif
