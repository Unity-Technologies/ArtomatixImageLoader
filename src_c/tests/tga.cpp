#include <gtest/gtest.h>
#include "../AIL.h"

#ifdef HAVE_TGA
#include <stdio.h>
#include <vector>
#include <string>
#include <setjmp.h>
#include <stdint.h>
#include "testCommon.h"

TEST(tga, TestDetectTGA)
{
    ASSERT_TRUE(detectImage("/tga/test.tga", TGA_IMAGE_FORMAT));
}

TEST(tga, TestDetectBadTGA)
{
    ASSERT_FALSE(detectImage("/jpeg/test.jpeg", TGA_IMAGE_FORMAT));
}


int main(int argc, char **argv)
{
    AImgInitialise();

    ::testing::InitGoogleTest(&argc, argv);
    int retval = RUN_ALL_TESTS();

    AImgCleanUp();

    return retval;
}
#endif
