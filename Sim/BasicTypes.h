#ifndef __BasicTypes_h__
#define __BasicTypes_h__

#include <cstdint>

using U8 = std::uint_fast8_t;
using U16 = std::uint_fast16_t;
using U32 = std::uint_fast32_t;

struct tSectorCount
{
    U32 _;

    operator U32() const { return _; }

    void operator=(U32 v)
    {
        _ = v;
    }

    tSectorCount operator++()
    {
        ++_;
        return *this;
    }

    tSectorCount operator++(int)
    {
        const tSectorCount old(*this);
        ++_;
        return old;
    }
};

struct tSectorOffset
{
    U32 _;

    operator U32() const { return _; }

    void operator=(U32 v)
    {
        _ = v;
    }

    tSectorOffset operator++()
    {
        ++_;
        return *this;
    }

    tSectorOffset operator++(int)
    {
        const tSectorOffset old(*this);
        ++_;
        return old;
    }
};

#endif