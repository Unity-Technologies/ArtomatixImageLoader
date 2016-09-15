#include <gtest/gtest.h>

#include <stdio.h>
#include <vector>
#include <string>

#include "../AIL.h"

template <typename T>
std::vector<T> readFile(const std::string& path)
{
    FILE* f = fopen(path.c_str(), "rb");
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    std::vector<T> retval(size/sizeof(T));
    fread(&retval[0], 1, size, f);

    fclose(f);
    
    return retval; 
}

std::string getImagesDir()
{
    std::string thisFile = __FILE__;
    std::string thisFolder = thisFile.substr(0, thisFile.size() - std::string("/exr.cpp").length());

    return thisFolder + "/images";
}

TEST(Exr, TestDetectExr)
{
    auto data = readFile<uint8_t>(getImagesDir() + "/exr/grad_32.exr");

    ReadCallback readCallback = NULL;
    TellCallback tellCallback = NULL;
    SeekCallback seekCallback = NULL;
    void* callbackData = NULL;

    AIGetSimpleMemoryBufferCallbacks(&readCallback, &tellCallback, &seekCallback, &callbackData, &data[0], data.size());

    AImgHandle img = NULL;
    int32_t fileFormat = UNKNOWN_IMAGE_FORMAT;
    AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &img, &fileFormat);

    ASSERT_EQ(fileFormat, EXR_IMAGE_FORMAT);

    AImgClose(img);
    AIDestroySimpleMemoryBufferCallbacks(readCallback, tellCallback, seekCallback, callbackData);
}

TEST(Exr, TestReadExrAttrs)
{
    auto data = readFile<uint8_t>(getImagesDir() + "/exr/grad_32.exr");

    ReadCallback readCallback = NULL;
    TellCallback tellCallback = NULL;
    SeekCallback seekCallback = NULL;
    void* callbackData = NULL;

    AIGetSimpleMemoryBufferCallbacks(&readCallback, &tellCallback, &seekCallback, &callbackData, &data[0], data.size());

    AImgHandle img = NULL;
    AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &img, NULL);

    int32_t width = 0;
    int32_t height = 0;
    int32_t numChannels = 0;
    int32_t bytesPerChannel = 0;
    int32_t floatOrInt = 0;
    int32_t decodedImgFormat = 0;

    AImgGetInfo(img, &width, &height, &numChannels, &bytesPerChannel, &floatOrInt, &decodedImgFormat);

    ASSERT_EQ(width, 64);
    ASSERT_EQ(height, 32);
    ASSERT_EQ(numChannels, 3);
    ASSERT_EQ(bytesPerChannel, 4);
    ASSERT_EQ(floatOrInt, AImgFloatOrIntType::FITYPE_FLOAT);
    ASSERT_EQ(decodedImgFormat, AImgFormat::RGB32F);


    AImgClose(img);
    AIDestroySimpleMemoryBufferCallbacks(readCallback, tellCallback, seekCallback, callbackData);
}

TEST(Exr, TestMemoryCallbacksRead)
{
    std::vector<uint8_t> data(10);
    for(size_t i = 0; i < data.size(); i++)
        data[i] = i;

    ReadCallback readCallback = NULL;
    TellCallback tellCallback = NULL;
    SeekCallback seekCallback = NULL;
    void* callbackData = NULL;

    AIGetSimpleMemoryBufferCallbacks(&readCallback, &tellCallback, &seekCallback, &callbackData, &data[0], data.size());

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

    AIDestroySimpleMemoryBufferCallbacks(readCallback, tellCallback, seekCallback, callbackData);
}

TEST(Exr, TestReadExr)
{
    auto data = readFile<uint8_t>(getImagesDir() + "/exr/grad_32.exr");

    ReadCallback readCallback = NULL;
    TellCallback tellCallback = NULL;
    SeekCallback seekCallback = NULL;
    void* callbackData = NULL;

    AIGetSimpleMemoryBufferCallbacks(&readCallback, &tellCallback, &seekCallback, &callbackData, &data[0], data.size());

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
    AIDestroySimpleMemoryBufferCallbacks(readCallback, tellCallback, seekCallback, callbackData);
}



int main(int argc, char **argv) 
{
    AImgInitialise();

    ::testing::InitGoogleTest(&argc, argv);
    int retval = RUN_ALL_TESTS();

    AImgCleanUp();

    return retval;
}
