#include "testCommon.h"
#include <cmath>
#include <string.h>

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
    int32_t err = AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &img, &fileFormat);

    if(img != NULL)
        AImgClose(img);

    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);

    if(err != AImgErrorCode::AIMG_SUCCESS)
        return false;

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
    uint32_t colourProfileLen = 0;

    AImgGetInfo(img, &width, &height, &numChannels, &bytesPerChannel, &floatOrInt, &decodedImgFormat, &colourProfileLen);
    char profileName[30];
    std::vector<uint8_t> colourProfile(colourProfileLen);
    AImgGetColourProfile(img, profileName, colourProfile.data(), &colourProfileLen);
    AImgClose(img);
    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);
    
    return (width == expectedWidth && height == expectedHeight && numChannels == expectedNumChannels && bytesPerChannel == expectedBytesPerChannel && floatOrInt == expectedFloatOrInt && decodedImgFormat == expectedFormat);
}

bool compareForceImageFormat(const std::string& path)
{
    auto data = readFile<uint8_t>(getImagesDir() + path);

    ReadCallback readCallback = NULL;
    WriteCallback writeCallback = NULL;
    TellCallback tellCallback = NULL;
    SeekCallback seekCallback = NULL;
    void* callbackData = NULL;

    AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &data[0], data.size());

    int32_t err = AIMG_SUCCESS;

    AImgHandle img = NULL;
    err = AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &img, NULL);
    if(err != AIMG_SUCCESS)
        return false;

    int32_t width = 0;
    int32_t height = 0;
    int32_t numChannels = 0;
    int32_t bytesPerChannel = 0;
    int32_t floatOrInt = 0;
    int32_t decodedImgFormat = 0;

    err = AImgGetInfo(img, &width, &height, &numChannels, &bytesPerChannel, &floatOrInt, &decodedImgFormat, NULL);
    if(err != AIMG_SUCCESS)
        return false;

    std::vector<uint16_t> data16(width*height*4);

    err = AImgDecodeImage(img, &data16[0], AImgFormat::RGBA16U);
    if(err != AIMG_SUCCESS)
        return false;

    AImgClose(img);
    img = NULL;

    seekCallback(callbackData, 0);

    err = AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &img, NULL);
    if(err != AIMG_SUCCESS)
        return false;

    std::vector<float> data32(width*height*4);

    err = AImgDecodeImage(img, &data32[0], AImgFormat::RGBA32F);
    if(err != AIMG_SUCCESS)
        return false;

    AImgClose(img);
    img = NULL;

    for(int32_t y = 0; y < height; y++)
    {
        for(int32_t x = 0; x < width; x++)
        {
            float r16 = ((float)data16[(x + y*width)*4 + 0]) / 65535.0f;
            float g16 = ((float)data16[(x + y*width)*4 + 1]) / 65535.0f;
            float b16 = ((float)data16[(x + y*width)*4 + 2]) / 65535.0f;
            float a16 = ((float)data16[(x + y*width)*4 + 3]) / 65535.0f;

            float r32 = data32[(x + y*width)*4 + 0];
            float g32 = data32[(x + y*width)*4 + 1];
            float b32 = data32[(x + y*width)*4 + 2];
            float a32 = data32[(x + y*width)*4 + 3];

            float diffR = std::abs(r32-r16);
            float diffG = std::abs(g32-g16);
            float diffB = std::abs(b32-b16);
            float diffA = std::abs(a32-a16);

            float threshold = 0.001f;

            if(diffR > threshold || diffG > threshold || diffB > threshold || diffA > threshold)
                return false;
        }
    }

    return true;
}

void writeToFile(const std::string& path, int32_t width, int32_t height, void* data, int32_t inputFormat, int32_t fileFormat, char *profileName, uint8_t *colourProfile, uint32_t colourProfileLen)
{
    ReadCallback readCallback = NULL;
    WriteCallback writeCallback = NULL;
    TellCallback tellCallback = NULL;
    SeekCallback seekCallback = NULL;
    void* callbackData = NULL;

    std::vector<uint8_t> fData;

    AIGetResizableMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &fData);

    AImgHandle wImg = AImgGetAImg(fileFormat);

    AImgWriteImage(wImg, data, width, height, inputFormat, profileName, colourProfile, colourProfileLen, writeCallback, tellCallback, seekCallback, callbackData, NULL);

    FILE* f = fopen(path.c_str(), "wb");
    fwrite(&fData[0], 1, fData.size(), f);
    fclose(f);

    AImgClose(wImg);
    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);

}

void readWriteIcc(const std::string & path, const std::string & outPath, char *profileName, uint8_t *colourProfile, uint32_t *colourProfileLen)
{
    // Read
    auto data = readFile<uint8_t>(getImagesDir() + path);

    ReadCallback readCallback = NULL;
    WriteCallback writeCallback = NULL;
    TellCallback tellCallback = NULL;
    SeekCallback seekCallback = NULL;
    void* callbackData = NULL;

    AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &data[0], data.size());

    AImgHandle img = NULL;
    int32_t detectedFormat;
    AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &img, &detectedFormat);

    int32_t width = 0;
    int32_t height = 0;
    int32_t numChannels = 0;
    int32_t bytesPerChannel = 0;
    int32_t floatOrInt = 0;
    int32_t decodedImgFormat = 0;

    AImgGetInfo(img, &width, &height, &numChannels, &bytesPerChannel, &floatOrInt, &decodedImgFormat, colourProfileLen);
    std::vector<uint8_t> colourProfile_(*colourProfileLen);
    AImgGetColourProfile(img, profileName, colourProfile_.data(), colourProfileLen);
    colourProfile = colourProfile_.data();
    
    std::vector<uint8_t> imgData;
    imgData.resize(width*height*numChannels * bytesPerChannel, 78);
    AImgDecodeImage(img, &imgData[0], AImgFormat::INVALID_FORMAT);
    
    AImgClose(img);
    img = NULL;
    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);
    readCallback = NULL; writeCallback = NULL; tellCallback = NULL, seekCallback = NULL; callbackData = NULL;
    
    // Write
    std::vector<uint8_t> fileData;
    
    fileData.resize(width * height * numChannels * bytesPerChannel * 5);
    AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &fileData[0], fileData.size());

    AImgHandle wImg = AImgGetAImg(detectedFormat);
    AImgWriteImage(wImg, &imgData[0], width, height, decodedImgFormat, profileName, colourProfile, *colourProfileLen, writeCallback, tellCallback, seekCallback, callbackData, NULL);
    
    FILE* f = fopen((getImagesDir() + outPath).c_str(), "wb");
    fwrite(&fileData[0], 1, fileData.size(), f);
    fclose(f);

    AImgClose(wImg);
    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);
}