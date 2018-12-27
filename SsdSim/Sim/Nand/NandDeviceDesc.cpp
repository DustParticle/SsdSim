#include "Nand/NandDeviceDesc.h"

NandDeviceDesc::NandDeviceDesc(U32 blockCount, U32 pagesPerBlock, U32 bytesPerPage) :
	_BlockCount(blockCount), _PagesPerBlock(pagesPerBlock), _BytesPerPage(bytesPerPage)
{

}