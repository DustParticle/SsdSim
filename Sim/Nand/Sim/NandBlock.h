#ifndef __NandBlock_h__
#define __NandBlock_h__

#include <memory>

#include "Nand/Types.h"
#include "Nand/Sim/NandBlockTracker.h"
#include "Nand/Sim/NandDeviceDesc.h"

class NandBlock
{
public:
	NandBlock(U32 pagesPerBlock, U32 totalBytesPerPage);
	NandBlock(NandBlock&& rhs) = default;

public:
	void Erase();

	void WritePage(tPageInBlock page, const U8* const pInData);
	void WritePage(const tPageInBlock& page, const tByteOffset& byteOffset, const tByteCount& byteCount, const U8* const inBuffer);

	bool ReadPage(tPageInBlock page, U8* const pOutData);
	bool ReadPage(const tPageInBlock& page, const tByteOffset& byteOffset, const tByteCount& byteCount, U8* const outBuffer);

public:
	static const U8 ERASED_PATTERN = 0xff;

private:
	std::unique_ptr<NandBlockTracker> _pNandBlockTracker;

	U32 _PagesPerBlock;
	U32 _TotalBytesPerPage;

	std::unique_ptr<U8[]> _Buffer;
	std::unique_ptr<U8[]> _ErasedBuffer;
};

#endif
