#ifndef __NandDeviceDesc_h__
#define __NandDeviceDesc_h__

#include "Common/BasicTypes.h"

class NandDeviceDesc
{
public:
	NandDeviceDesc(U32 blockCount, U32 pagesPerBlock, U32 bytesPerPage);

public:
	inline U32 GetBlockCount() const { return _BlockCount; }
	inline U32 GetPagesPerBlock() const { return _PagesPerBlock; }
	inline U32 GetBytesPerPage() const { return _BytesPerPage; }

private:
	U32	_BlockCount;
	U32 _PagesPerBlock;
	U32 _BytesPerPage;
};

#endif

