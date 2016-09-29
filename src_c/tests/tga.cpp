#include <gtest/gtest.h>
#include "../AIL.h"

#ifdef HAVE_TGA
#include <stdio.h>
#include <vector>
#include <string>
#include <setjmp.h>
#include <stdint.h>
#include "testCommon.h"

int main(int argc, char **argv)
{
    AImgInitialise();

    ::testing::InitGoogleTest(&argc, argv);
    int retval = RUN_ALL_TESTS();

    AImgCleanUp();

    return retval;
}
#endif
