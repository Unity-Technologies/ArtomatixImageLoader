#include <gtest/gtest.h>
#include "testCommon.h"

#include "../AIL.h"

#ifdef HAVE_EXR

TEST(Exr, TestDetectExr)
{
    ASSERT_TRUE(detectImage("/exr/grad_32.exr", EXR_IMAGE_FORMAT));
}

TEST(Exr, TestReadExrAttrs)
{
    ASSERT_TRUE(validateImageHeaders("/exr/grad_32.exr", 64, 32, 3, 4, AImgFloatOrIntType::FITYPE_FLOAT, AImgFormat::RGB32F));
}


// THIS HAS NOTHING TO DO WITH EXRS

TEST(Exr, TestMemoryCallbacksRead)
{
    std::vector<uint8_t> data(10);
    for(size_t i = 0; i < data.size(); i++)
        data[i] = i;

    ReadCallback readCallback = NULL;
    WriteCallback writeCallback = NULL;
    TellCallback tellCallback = NULL;
    SeekCallback seekCallback = NULL;
    void* callbackData = NULL;

    AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &data[0], data.size());

    std::vector<uint8_t> readBuf(data.size());
    std::fill(readBuf.begin(), readBuf.end(), 100);

    int32_t read = readCallback(callbackData, &readBuf[0], 12);
    ASSERT_EQ(read, 10);
    for(size_t i = 0; i < data.size(); i++)
        ASSERT_EQ(readBuf[i], i);

    std::fill(readBuf.begin(), readBuf.end(), 100);
    seekCallback(callbackData, 1);
    read = readCallback(callbackData, &readBuf[0], 12);
    ASSERT_EQ(read, 9);
    for(size_t i = 0; i < data.size()-1; i++)
        ASSERT_EQ(readBuf[i], i+1);

    seekCallback(callbackData, 0);
    ASSERT_EQ(tellCallback(callbackData), 0);

    std::fill(readBuf.begin(), readBuf.end(), 100);
    readCallback(callbackData, &readBuf[0], 1);
    ASSERT_EQ(tellCallback(callbackData), 1);
    ASSERT_EQ(readBuf[0], 0);
    ASSERT_EQ(readBuf[1], 100);

    std::fill(readBuf.begin(), readBuf.end(), 100);
    readCallback(callbackData, &readBuf[0], 2);
    ASSERT_EQ(tellCallback(callbackData), 3);
    ASSERT_EQ(readBuf[0], 1);
    ASSERT_EQ(readBuf[1], 2);
    ASSERT_EQ(readBuf[2], 100);

    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);
}

TEST(Exr, TestReadExr)
{
    auto data = readFile<uint8_t>(getImagesDir() + "/exr/grad_32.exr");

    ReadCallback readCallback = NULL;
    WriteCallback writeCallback = NULL;
    TellCallback tellCallback = NULL;
    SeekCallback seekCallback = NULL;
    void* callbackData = NULL;

    AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &data[0], data.size());

    AImgHandle img = NULL;
    AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &img, NULL);

    int32_t width = 64;
    int32_t height = 32;

    std::vector<float> imgData(width*height*3, 0.0f);

    AImgDecodeImage(img, &imgData[0], AImgFormat::INVALID_FORMAT);

    auto knownData = readFile<float>(getImagesDir() + "/exr/grad_32.bin");

    for(int32_t y = 0; y < height; y++)
    {
        for(int32_t x = 0; x < width; x++)
        {
            ASSERT_EQ(knownData[x + width*y], imgData[(x + width*y) * 3]);
        }
    }

    AImgClose(img);
    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);
}

TEST(Exr, TesWriteExr)
{
    auto data = readFile<uint8_t>(getImagesDir() + "/exr/grad_32.exr");

    ReadCallback readCallback = NULL;
    WriteCallback writeCallback = NULL;
    TellCallback tellCallback = NULL;
    SeekCallback seekCallback = NULL;
    void* callbackData = NULL;

    AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &data[0], data.size());

    AImgHandle img = NULL;
    AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &img, NULL);

    int32_t width = 64;
    int32_t height = 32;

    std::vector<float> imgData(width*height*3, 0.0f);

    AImgDecodeImage(img, &imgData[0], AImgFormat::INVALID_FORMAT);
    AImgClose(img);
    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);

    std::vector<char> fileData(4096); // fixed size buffer for a file write, not the best but it'll do for this test
    AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &fileData[0], fileData.size());

    AImgHandle wImg = AImgGetAImg(AImgFileFormat::EXR_IMAGE_FORMAT);
    AImgWriteImage(wImg, &imgData[0], width, height, AImgFormat::RGB32F, writeCallback, tellCallback, seekCallback, callbackData);
    AImgClose(wImg);

    seekCallback(callbackData, 0);


    AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &img, NULL);
    std::vector<float> imgData2(width*height*3, 0.0f);
    AImgDecodeImage(img, &imgData2[0], AImgFormat::INVALID_FORMAT);
    AImgClose(img);
    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);

    for(uint32_t i = 0; i < imgData2.size(); i++)
        ASSERT_EQ(imgData[i], imgData2[i]);
}

TEST(Exr, TestConvertDataFormat)
{
    int32_t width = 16;
    int32_t height = 16;

    std::vector<uint8_t> startingData(width*height);

    for(int32_t y = 0; y < height; y++)
    {
        for(int32_t x = 0; x < width; x++)
        {
            startingData[x + y*width] = x+y;
        }
    }

    std::vector<float> convertedF(width*height*4, 0);

    AImgConvertFormat(&startingData[0], &convertedF[0], width, height, AImgFormat::R8U, AImgFormat::RGBA32F);

    for(int32_t y = 0; y < height; y++)
    {
        for(int32_t x = 0; x < width; x++)
        {
            ASSERT_EQ((uint8_t)(convertedF[(x + y*width) * 4] * 255.0f), startingData[x + y*width]);
        }
    }

    std::vector<uint8_t> convertedBack(width*height, 0);

    AImgConvertFormat(&convertedF[0], &convertedBack[0], width, height, AImgFormat::RGBA32F, AImgFormat::R8U);

    for(uint32_t i = 0; i < convertedBack.size(); i++)
        ASSERT_EQ(startingData[i], convertedBack[i]);
}

#endif

int main(int argc, char **argv) 
{
    AImgInitialise();

    ::testing::InitGoogleTest(&argc, argv);
    int retval = RUN_ALL_TESTS();

    AImgCleanUp();

    return retval;
}
