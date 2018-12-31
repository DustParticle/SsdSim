#include "Nand/NandChannel.h"

void NandChannel::Init(U8 deviceCount, U32 blocksPerDevice, U32 pagesPerBlock, U32 bytesPerPage)
{
	for (U8 i(0); i < deviceCount; ++i)
	{
		_Devices.push_back(std::move(NandDevice(blocksPerDevice, pagesPerBlock, bytesPerPage)));
	}
}

NandDevice& NandChannel::operator[](const int index)
{
	return _Devices[index];
}