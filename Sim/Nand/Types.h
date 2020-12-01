#ifndef __Types_h__
#define __Types_h__

#include "BasicTypes.h"

struct tChannel
{
	U8 _;

    operator U8() const { return _; }

    void operator=(U8 v)
    {
        _ = v;
    }

    tChannel operator++()
    {
        ++_;
        return *this;
    }

    tChannel operator++(int)
    {
        const tChannel old(*this);
        ++_;
        return old;
    }
};

struct tDeviceInChannel
{
	U8 _;

    operator U8() const { return _; }

    void operator=(U8 v)
    {
        _ = v;
    }

    tDeviceInChannel operator++()
    {
        ++_;
        return *this;
    }

    tDeviceInChannel operator++(int)
    {
        const tDeviceInChannel old(*this);
        ++_;
        return old;
    }
};

struct tBlockInDevice
{
	U16 _;

    operator U16() const { return _; }
    
    void operator=(U16 v)
    {
        _ = v;
    }

    tBlockInDevice operator++()
    {
        ++_;
        return *this;
    }

    tBlockInDevice operator++(int)
    {
        const tBlockInDevice old(*this);
        ++_;
        return old;
    }
};

struct tPageInBlock
{
	U16 _;

    operator U16() const { return _; }

    void operator=(U16 v)
    {
        _ = v;
    }

    tPageInBlock operator++()
    {
        ++_;
        return *this;
    }

    tPageInBlock operator++(int)
    {
        const tPageInBlock old(*this);
        ++_;
        return old;
    }
};

struct tSectorInPage
{
	U8	_;

    operator U8() const { return _; }

    void operator=(U8 v)
    {
        _ = v;
    }

    tSectorInPage operator++()
    {
        ++_;
        return *this;
    }

    tSectorInPage operator++(int)
    {
        const tSectorInPage old(*this);
        ++_;
        return old;
    }
};

#endif