#include "testCommon.h"

bool detectImage(const std::string& path, int32_t format)
{
    auto data = readFile<uint8_t>(getImagesDir() + path);

    ReadCallback readCallback = NULL;
    WriteCallback writeCallback = NULL;
    TellCallback tellCallback = NULL;
    SeekCallback seekCallback = NULL;
    void* callbackData = NULL;

    AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &data[0], data.size());

    AImgHandle img = NULL;
    int32_t fileFormat = UNKNOWN_IMAGE_FORMAT;
    AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &img, &fileFormat);

    AImgClose(img);
    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);

    return fileFormat == format;
}

bool validateImageHeaders(const std::string & path, int32_t expectedWidth, int32_t expectedHeight, int32_t expectedNumChannels, int32_t expectedBytesPerChannel, int32_t expectedFloatOrInt, int32_t expectedFormat)
{
    auto data = readFile<uint8_t>(getImagesDir() + path);

    ReadCallback readCallback = NULL;
    WriteCallback writeCallback = NULL;
    TellCallback tellCallback = NULL;
    SeekCallback seekCallback = NULL;
    void* callbackData = NULL;

    AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &data[0], data.size());

    AImgHandle img = NULL;
    AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &img, NULL);

    int32_t width = 0;
    int32_t height = 0;
    int32_t numChannels = 0;
    int32_t bytesPerChannel = 0;
    int32_t floatOrInt = 0;
    int32_t decodedImgFormat = 0;

    AImgGetInfo(img, &width, &height, &numChannels, &bytesPerChannel, &floatOrInt, &decodedImgFormat);
    AImgClose(img);
    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);

    return (width == expectedWidth && height == expectedHeight && numChannels == expectedNumChannels && bytesPerChannel == expectedBytesPerChannel && floatOrInt == expectedFloatOrInt && decodedImgFormat == expectedFormat);
}
