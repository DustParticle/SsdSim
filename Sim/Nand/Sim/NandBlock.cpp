#include <cstring>
#include <assert.h>

#include "Nand/Sim/NandBlock.h"

NandBlock::NandBlock(U32 pagesPerBlock, U32 totalBytesPerPage)
{
	_PagesPerBlock = pagesPerBlock;
	_TotalBytesPerPage = totalBytesPerPage;

	_ErasedBuffer = std::unique_ptr<U8[]>(new U8[_TotalBytesPerPage]);
	std::memset(_ErasedBuffer.get(), ERASED_PATTERN, _TotalBytesPerPage);
}

void NandBlock::Erase()
{
	_Buffer.reset();
}

void NandBlock::WritePage(tPageInBlock page, const U8* const pInData)
{
	if (nullptr == _Buffer)
	{
		_Buffer = std::unique_ptr<U8[]>(new U8[_PagesPerBlock * _TotalBytesPerPage]);
	}

	std::memcpy((void*)&_Buffer[page._ * _TotalBytesPerPage], pInData, _TotalBytesPerPage);
}

void NandBlock::WritePage(const tPageInBlock& page, const tByteOffset& byteOffset, const tByteCount& byteCount, const U8* const inBuffer)
{
	assert(byteCount._ > 0);
	assert((byteOffset._ + byteCount._) <= _TotalBytesPerPage);

	if (nullptr == _Buffer)
	{
		_Buffer = std::unique_ptr<U8[]>(new U8[_PagesPerBlock * _TotalBytesPerPage]);
	}

	std::memcpy((void*)&_Buffer[(page._ * _TotalBytesPerPage) + byteOffset._], inBuffer, byteCount._);
}

void NandBlock::ReadPage(tPageInBlock page, U8* const pOutData)
{
	auto pData = (nullptr == _Buffer) ? &_ErasedBuffer[0] : &_Buffer[page._ * _TotalBytesPerPage];

	std::memcpy((void*)pOutData, (void*)pData, _TotalBytesPerPage);
}

void NandBlock::ReadPage(const tPageInBlock& page, const tByteOffset& byteOffset, const tByteCount& byteCount, U8* const outBuffer)
{
	assert(byteCount._ > 0);
	assert((byteOffset._ + byteCount._) <= _TotalBytesPerPage);

	auto data = (nullptr == _Buffer) ? &_ErasedBuffer[0] : &_Buffer[(page._ * _TotalBytesPerPage) + byteOffset._];

	std::memcpy((void*)outBuffer, (void*)data, byteCount._);
}

