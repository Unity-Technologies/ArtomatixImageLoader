#include "testCommon.h"
#include <gtest/gtest.h>
#include "../AIL.h"
#define STBI_ONLY_HDR
#define STB_IMAGE_IMPLEMENTATION
#include "../extern/stb_image.h"

std::vector<uint8_t> decodeHDRFile(const std::string & path)
{
    int width, height, comp;
    FILE * file = fopen(path.c_str(), "rb");

    stbi_hdr_to_ldr_gamma(1.0f);
    stbi_ldr_to_hdr_gamma(1.0f);

    uint8_t * loadedData = stbi_load_from_file(file, &width, &height, &comp, 0);
    std::vector<uint8_t> data(width * height * comp);

    memcpy(&data[0], loadedData, width*height*comp);
    stbi_image_free(loadedData);
    fclose(file);

    return data;
}

TEST(HDR, TestDetectHDR)
{
    ASSERT_TRUE(detectImage("/hdr/test-env.hdr", HDR_IMAGE_FORMAT));
}

TEST(HDR, TestDetectBadHDR)
{
    ASSERT_FALSE(detectImage("/jpeg/test.jpeg", HDR_IMAGE_FORMAT));
}

TEST(HDR, TestReadHDRFile)
{
    auto data = readFile<uint8_t>(getImagesDir() + "/hdr/test-env.hdr");

    ReadCallback readCallback = NULL;
    WriteCallback writeCallback = NULL;
    TellCallback tellCallback = NULL;
    SeekCallback seekCallback = NULL;
    void* callbackData = NULL;

    AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &data[0], (int32_t)data.size());

    AImgHandle img = NULL;
    AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &img, NULL);

    int32_t width;
    int32_t height;
    int32_t numChannels;
    int32_t bytesPerChannel;
    int32_t floatOrInt;
    int32_t imgFmt;

    AImgGetInfo(img, &width, &height, &numChannels, &bytesPerChannel, &floatOrInt, &imgFmt, NULL);

    std::vector<uint8_t> imgData(width*height*numChannels*bytesPerChannel, 78);

    int32_t error = AImgDecodeImage(img, &imgData[0], AImgFormat::INVALID_FORMAT);

    if (error != AImgErrorCode::AIMG_SUCCESS)
    {
        std::cout << AImgGetErrorDetails(img) << std::endl;
    }

    auto knownData = decodeHDRFile(getImagesDir() + "/hdr/test-env.hdr");

    auto ptr = knownData.data();
    for (int32_t y = 0; y < height; y++)
    {
        for (int32_t x = 0; x < width; x++)
        {
            if (knownData[x + width*y] != imgData[(x + width*y)])
            {
                break;
            }
            ASSERT_EQ(knownData[x + width*y], imgData[(x + width*y)]);
        }
    }

    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);
    AImgClose(img);
}

int main(int argc, char * argv[])
{
    AImgInitialise();

    ::testing::InitGoogleTest(&argc, argv);
    int retval = RUN_ALL_TESTS();

    AImgCleanUp();

    return retval;
}