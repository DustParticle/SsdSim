#ifndef __BasicTypes_h__
#define __BasicTypes_h__

#include <cstdint>

using U8 = std::uint_fast8_t;
using U16 = std::uint_fast16_t;
using U32 = std::uint_fast32_t;

template<class T, class N>
struct PrimitiveTemplate
{
    operator T() const { return _; }

    void operator=(T v)
    {
        _ = v;
    }

    PrimitiveTemplate<T, N> operator++()
    {
        ++_;
        return *this;
    }

    PrimitiveTemplate<T, N> operator++(int)
    {
        const PrimitiveTemplate<T, N> old(*this);
        ++_;
        return old;
    }

    T _;
};

using tSectorCount = PrimitiveTemplate<U32, struct SectorCount>;
using tSectorOffset = PrimitiveTemplate<U32, struct SectorOffset>;

#endif