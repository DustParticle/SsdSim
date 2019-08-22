#pragma once
#ifndef __NandChannel_h__
#define __NandChannel_h__

#include <vector>

#include "Nand/Sim/NandDevice.h"

#include "Buffer/Hal/BufferHal.h"

class NandChannel
{
public:
	void Init(BufferHal *bufferHal, U8 deviceCount, U32 blocksPerDevice, U32 pagesPerBlock, U32 bytesPerPage);

public:
	NandDevice& operator[](const int index);

private:
	std::vector<NandDevice> _Devices;
};

#endif