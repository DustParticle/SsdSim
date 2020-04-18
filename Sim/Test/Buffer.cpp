#include "pch.h"

#include "Buffer/Hal/BufferHal.h"

TEST(BufferHal, Basic)
{
    constexpr U32 bufferSizeInKB = 16;
    constexpr U32 bufferSizeInSector = bufferSizeInKB * 2;
    constexpr U32 requestingBufferSize = 10;
    constexpr U32 requestingBufferSizeInByte = 10 * 512;

    BufferHal bufferHal;
    bufferHal.PreInit(bufferSizeInKB);

    Buffer buffer;
    ASSERT_TRUE(bufferHal.AllocateBuffer(BufferType::User, requestingBufferSize, buffer));
    ASSERT_EQ(buffer.SizeInSector, requestingBufferSize);
    ASSERT_NE(bufferHal.ToPointer(buffer), nullptr);

    U8 tempBuffer[requestingBufferSizeInByte];
    for (U8 i = 0; i < requestingBufferSize; ++i)
    {
        memset(&tempBuffer[i * 512], i, 512);
    }
    tSectorCount sectorCount;
    sectorCount._ = buffer.SizeInSector;
    tSectorOffset sectorOffset;
    sectorOffset._ = 0;
    bufferHal.Memcpy(buffer, sectorOffset, tempBuffer, sectorCount);
    ASSERT_EQ(memcmp(bufferHal.ToPointer(buffer), tempBuffer, requestingBufferSizeInByte), 0);

    U8 tempBuffer1[requestingBufferSizeInByte];
    bufferHal.Memcpy(tempBuffer1, buffer, sectorOffset, sectorCount);
    ASSERT_EQ(memcmp(tempBuffer, tempBuffer1, requestingBufferSizeInByte), 0);

    ASSERT_NO_THROW(bufferHal.DeallocateBuffer(buffer));
}

TEST(BufferHal, Exception)
{
    constexpr U32 bufferSizeInKB = 16;
    constexpr U32 bufferSizeInSector = bufferSizeInKB * 2;

    BufferHal bufferHal;
    bufferHal.PreInit(bufferSizeInKB);

    Buffer buffer;
    ASSERT_FALSE(bufferHal.AllocateBuffer(BufferType::User, bufferSizeInSector + 1, buffer));     // Request a buffer that larger than BufferHal pool
    EXPECT_DEATH(bufferHal.DeallocateBuffer(buffer), "");                       // The bufferHal will assert
    ASSERT_EQ(bufferHal.ToPointer(buffer), nullptr);

    Buffer buffer1, buffer2;
    ASSERT_TRUE(bufferHal.AllocateBuffer(BufferType::User, bufferSizeInSector - 1, buffer1));
    ASSERT_FALSE(bufferHal.AllocateBuffer(BufferType::User, 2, buffer2));                         // Don't have enough buffer
    ASSERT_TRUE(bufferHal.AllocateBuffer(BufferType::User, 1, buffer2));
    ASSERT_NO_THROW(bufferHal.DeallocateBuffer(buffer1));
    
    ASSERT_TRUE(bufferHal.AllocateBuffer(BufferType::User, bufferSizeInSector - 3, buffer1));     // Request new buffer
    ASSERT_NO_THROW(bufferHal.DeallocateBuffer(buffer1));
    ASSERT_NO_THROW(bufferHal.DeallocateBuffer(buffer2));
}