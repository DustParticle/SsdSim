//
// gtest-cout.h
// Work around to support print messages for gtest
// Reference: https://stackoverflow.com/a/45344932
//

#ifndef _GTEST_COUT_H_
#define _GTEST_COUT_H_

#pragma once

#include "gtest/gtest.h"

namespace testing
{
    namespace internal
    {
        enum GTestColor
        {
            COLOR_DEFAULT, COLOR_RED, COLOR_GREEN, COLOR_YELLOW
        };
        extern void ColoredPrintf(GTestColor color, const char* fmt, ...);
    }
}

#define GOUT(STREAM) \
    do \
    { \
        std::stringstream ss; \
        ss << STREAM << std::endl; \
        testing::internal::ColoredPrintf(testing::internal::COLOR_GREEN, "[          ] "); \
        testing::internal::ColoredPrintf(testing::internal::COLOR_DEFAULT, ss.str().c_str()); \
    } while (false); \

#endif /* _GTEST_COUT_H_ */