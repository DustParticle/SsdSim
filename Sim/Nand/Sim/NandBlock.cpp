#include <cstring>
#include <assert.h>

#include "Nand/Sim/NandBlock.h"

NandBlock::NandBlock(U32 pagesPerBlock, U32 totalBytesPerPage)
{
	_PagesPerBlock = pagesPerBlock;
	_TotalBytesPerPage = totalBytesPerPage;

	_ErasedBuffer = std::unique_ptr<U8[]>(new U8[_TotalBytesPerPage]);
	std::memset(_ErasedBuffer.get(), ERASED_PATTERN, _TotalBytesPerPage);

	// Initialize the NandBlockTracker
	_pNandBlockTracker = make_unique<NandBlockTracker>(pagesPerBlock);
}

void NandBlock::Erase()
{
	_Buffer.reset();
	_pNandBlockTracker->Reset();
}

void NandBlock::WritePage(tPageInBlock page, const U8* const pInData)
{
	if (nullptr == _Buffer)
	{
		_Buffer = std::unique_ptr<U8[]>(new U8[_PagesPerBlock * _TotalBytesPerPage]);
	}

	std::memcpy((void*)&_Buffer[page._ * _TotalBytesPerPage], pInData, _TotalBytesPerPage);
	_pNandBlockTracker->WritePage(page);
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
	_pNandBlockTracker->WritePage(page);
}

bool NandBlock::ReadPage(tPageInBlock page, U8* const pOutData)
{
    // If page is written then return false to indicate ReadPage failed
    if (true == _pNandBlockTracker->IsPageWritten(page))
    {
        return (false);
    }

	auto pData = (nullptr == _Buffer) ? &_ErasedBuffer[0] : &_Buffer[page._ * _TotalBytesPerPage];

	std::memcpy((void*)pOutData, (void*)pData, _TotalBytesPerPage);
	return (true);
}

bool NandBlock::ReadPage(const tPageInBlock& page, const tByteOffset& byteOffset, const tByteCount& byteCount, U8* const outBuffer)
{
    // If page is written then return false to indicate ReadPage failed
    if (true == _pNandBlockTracker->IsPageWritten(page))
    {
        return (false);
    }

	assert(byteCount._ > 0);
	assert((byteOffset._ + byteCount._) <= _TotalBytesPerPage);

	auto data = (nullptr == _Buffer) ? &_ErasedBuffer[0] : &_Buffer[(page._ * _TotalBytesPerPage) + byteOffset._];

	std::memcpy((void*)outBuffer, (void*)data, byteCount._);
	return (true);
}

