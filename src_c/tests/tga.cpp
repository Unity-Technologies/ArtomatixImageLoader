#include <gtest/gtest.h>
#include "../AIL.h"

#ifdef HAVE_TGA
#include <stdio.h>
#include <vector>
#include <string>
#include <setjmp.h>
#include <stdint.h>
#include "testCommon.h"

#define STBI_ONLY_TGA
#define STB_IMAGE_IMPLEMENTATION
#include "../extern/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../extern/stb_image_write.h"

std::vector<uint8_t> decodeTGAFile(const std::string & path)
{
    int width, height, comp;
    FILE * file = fopen(path.c_str(), "rb");

    uint8_t * loadedData = stbi_load_from_file(file, &width, &height, &comp, 0);
    std::vector<uint8_t> data(width * height * comp);

    memcpy(&data[0], loadedData, width*height*comp);
    stbi_image_free(loadedData);
    fclose(file);

    return data;
}

TEST(TGA, TestDetectTGA)
{
    ASSERT_TRUE(detectImage("/tga/test.tga", TGA_IMAGE_FORMAT));
}

TEST(TGA, TestDetectTGAIndexed)
{
    ASSERT_TRUE(detectImage("/tga/indexed.tga", TGA_IMAGE_FORMAT));
}

TEST(TGA, TestDetectBadTGA)
{
    ASSERT_FALSE(detectImage("/jpeg/test.jpeg", TGA_IMAGE_FORMAT));
}

TEST(TGA, TestReadGoodTGAAttrs)
{
    ASSERT_TRUE(validateImageHeaders("/tga/test.tga", 640, 400, 3, 1, AImgFloatOrIntType::FITYPE_INT, AImgFormat::RGB8U));
}

TEST(TGA, TestReadBadTGAAttrs)
{
    ASSERT_FALSE(validateImageHeaders("/tga/test.tga", 640, 400, 3, 1, AImgFloatOrIntType::FITYPE_FLOAT, AImgFormat::RGBA16F));
}

TEST(TGA, TestCompareForceImageFormat1)
{
    ASSERT_TRUE(compareForceImageFormat("/tga/test.tga"));
}

TEST(TGA, TestCompareForceImageFormat2)
{
    ASSERT_TRUE(compareForceImageFormat("/tga/indexed.tga"));
}

TEST(TGA, TestReadTGAFile)
{
    auto data = readFile<uint8_t>(getImagesDir() + "/tga/test.tga");

    ReadCallback readCallback = NULL;
    WriteCallback writeCallback = NULL;
    TellCallback tellCallback = NULL;
    SeekCallback seekCallback = NULL;
    void* callbackData = NULL;

    AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &data[0], data.size());

    AImgHandle img = NULL;
    AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &img, NULL);

    int32_t width;
    int32_t height;
    int32_t numChannels;
    int32_t bytesPerChannel;
    int32_t floatOrInt;
    int32_t imgFmt;

    AImgGetInfo(img, &width, &height, &numChannels, &bytesPerChannel, &floatOrInt, &imgFmt);

    std::vector<uint8_t> imgData(width*height*numChannels*bytesPerChannel, 78);

    int32_t error = AImgDecodeImage(img, &imgData[0], AImgFormat::INVALID_FORMAT);

    if (error != AImgErrorCode::AIMG_SUCCESS)
    {
        std::cout << AImgGetErrorDetails(img) << std::endl;
    }

    auto knownData = decodeTGAFile(getImagesDir() + "/tga/test.tga");

    for(int32_t y = 0; y < height; y++)
    {
        for(int32_t x = 0; x < width; x++)
        {
            ASSERT_EQ(knownData[x + width*y], imgData[(x + width*y)]);
        }
    }

    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);
    AImgClose(img);
}


TEST(TGA, TestWriteTGAFile)
{
    auto data = readFile<uint8_t>(getImagesDir() + "/tga/test.tga");

    ReadCallback readCallback = NULL;
    WriteCallback writeCallback = NULL;
    TellCallback tellCallback = NULL;
    SeekCallback seekCallback = NULL;
    void* callbackData = NULL;

    AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &data[0], data.size());

    AImgHandle img = NULL;
    AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &img, NULL);

    int32_t width;
    int32_t height;
    int32_t numChannels;
    int32_t bytesPerChannel;
    int32_t floatOrInt;
    int32_t fmt;

    AImgGetInfo(img, &width, &height, &numChannels, &bytesPerChannel, &floatOrInt, &fmt);

    std::vector<uint8_t> imgData(width*height*numChannels * bytesPerChannel, 78);

    AImgDecodeImage(img, &imgData[0], AImgFormat::INVALID_FORMAT);

    AImgClose(img);
    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);

    std::vector<char> fileData(width * height * numChannels * bytesPerChannel * 5);
    AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &fileData[0], fileData.size());

    AImgHandle wImg = AImgGetAImg(AImgFileFormat::TGA_IMAGE_FORMAT);
    auto err = AImgWriteImage(wImg, &imgData[0], width, height, fmt, writeCallback, tellCallback, seekCallback, callbackData);
    ASSERT_EQ(err, AImgErrorCode::AIMG_SUCCESS);
    AImgClose(wImg);

    seekCallback(callbackData, 0);

    AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &img, NULL);

    std::vector<uint8_t> imgData2(width*height*numChannels*bytesPerChannel, 0);
    AImgDecodeImage(img, &imgData2[0], AImgFormat::INVALID_FORMAT);
    AImgClose(img);
    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);

    for(uint32_t i = 0; i < imgData2.size(); i++)
       ASSERT_EQ(imgData[i], imgData2[i]);
}

TEST(TGA, TestForceImageFormat)
{
    auto data = readFile<uint8_t>(getImagesDir() + "/tga/test.tga");

    ReadCallback readCallback = NULL;
    WriteCallback writeCallback = NULL;
    TellCallback tellCallback = NULL;
    SeekCallback seekCallback = NULL;
    void* callbackData = NULL;

    AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &data[0], data.size());

    AImgHandle img = NULL;
    AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &img, NULL);


    int32_t width;
    int32_t height;
    int32_t numChannels;
    int32_t bytesPerChannel;
    int32_t floatOrInt;
    int32_t imgFmt;
    AImgGetInfo(img, &width, &height, &numChannels, &bytesPerChannel, &floatOrInt, &imgFmt);


    std::vector<float> imgData(width*height*3, 78);

    int32_t error = AImgDecodeImage(img, &imgData[0], AImgFormat::RGB32F);
    if (error != AImgErrorCode::AIMG_SUCCESS)
    {
        std::cout << AImgGetErrorDetails(img) << std::endl;
    }

    AImgClose(img);
    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);

    auto knownData = decodeTGAFile(getImagesDir() + "/tga/test.tga");


    for(uint32_t i = 0; i < imgData.size(); i++)
        ASSERT_EQ(((float)knownData[i]) / 255.0f, imgData[i]);

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
