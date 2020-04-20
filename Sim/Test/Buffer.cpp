#include "pch.h"

#include "Test/gtest-cout.h"

#include "Buffer/Hal/BufferHal.h"

TEST(BufferHal, Basic)
{
    constexpr U32 bufferSizeInKB = 16;
    constexpr U32 bufferSizeInSector = bufferSizeInKB * 2;
    constexpr U32 requestingBufferSize = 10;
    constexpr U32 bytesPerSector = 512;
    constexpr U32 requestingBufferSizeInByte = 10 * bytesPerSector;

    BufferHal bufferHal;
    bufferHal.PreInit(bufferSizeInKB);

    Buffer buffer;
    ASSERT_TRUE(bufferHal.AllocateBuffer(BufferType::User, requestingBufferSize, buffer));
    ASSERT_EQ(buffer.SizeInSector, requestingBufferSize);
    ASSERT_NE(bufferHal.ToPointer(buffer), nullptr);

    U8 source[requestingBufferSizeInByte];
    for (U8 i = 0; i < requestingBufferSize; ++i)
    {
        memset(&source[i * bytesPerSector], 0xa0 + i, bytesPerSector);
    }

    U8 dest[requestingBufferSizeInByte];

    U8 zeros[requestingBufferSizeInByte];
    memset(&zeros, 0, requestingBufferSizeInByte);

    U8 comparer[requestingBufferSizeInByte];

    tSectorCount sectorCount;
    tSectorOffset sectorOffset;
    sectorOffset._ = 0;
    for ( ; sectorOffset._ < (buffer.SizeInSector - 1); ++sectorOffset._)
    {
        sectorCount._ = 1;
        for (; sectorCount._ < buffer.SizeInSector - sectorOffset._; ++sectorCount._)
        {
            tSectorOffset zso{ 0 };
            tSectorCount zsc{ buffer.SizeInSector };
            bufferHal.CopyToBuffer(zeros, buffer, zso, zsc);
            ASSERT_EQ(memcmp(bufferHal.ToPointer(buffer), zeros, requestingBufferSizeInByte), 0);

            memset(&comparer, 0, requestingBufferSizeInByte);
            memcpy(comparer + (sectorOffset._ * bytesPerSector), source, sectorCount._ * bytesPerSector);

            bufferHal.CopyToBuffer(source, buffer, sectorOffset, sectorCount);
            if (0 != memcmp(bufferHal.ToPointer(buffer), comparer, requestingBufferSizeInByte))
            {
                GOUT("Memcpy to buffer miscompared!");
                GOUT("SectorOffset " << sectorOffset._);
                GOUT("SectorCount " << sectorCount._);

                FAIL();
            }

            memset(&comparer, 0, requestingBufferSizeInByte);
            memcpy(comparer, source, sectorCount._ * bytesPerSector);

            memset(&dest, 0, requestingBufferSizeInByte);
            bufferHal.CopyFromBuffer(dest, buffer, sectorOffset, sectorCount);
            if (0 != memcmp(comparer, dest, requestingBufferSizeInByte))
            {
                GOUT("Memcpy from buffer miscompared!");
                GOUT("SectorOffset " << sectorOffset._);
                GOUT("SectorCount " << sectorCount._);
                
                FAIL();
            }
        }
    }

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