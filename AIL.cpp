#include <map>
#include <vector>
#include <iostream>
#include <cstring>

#include "AIL.h"
#include "AIL_internal.h"

#include "exr.h"

#define HAVE_EXR // TODO: generate this in the cmake project files

#ifdef HAVE_EXR
    #include <half.h>
#endif

std::map<int32_t, AImg::ImageLoaderBase*> loaders;
std::string lastError;

void AISetLastErrorDetails(const char* err)
{
    if(err == NULL)
        err = "";

    lastError = std::string(err);
}

const char* AIGetLastErrorDetails()
{
    return lastError.c_str();
}


int32_t AImgInitialise()
{
    #ifdef HAVE_EXR
        loaders[AImgFileFormat::EXR_IMAGE_FORMAT] = new AImg::ExrImageLoader();
    #endif

    for(auto it = loaders.begin(); it != loaders.end(); ++it)
    {
        int32_t err = it->second->initialise();
        if(err != AImgErrorCode::AIMG_SUCCESS)
            return err;
    }

    return AImgErrorCode::AIMG_SUCCESS;
}

void AImgCleanUp()
{
    for(auto it = loaders.begin(); it != loaders.end(); ++it)
        delete it->second;

    loaders.clear();
}

namespace AImg
{
    AImgBase::~AImgBase() {} // go away c++
    ImageLoaderBase::~ImageLoaderBase() {}
}

int32_t AImgOpen(ReadCallback readCallback, TellCallback tellCallback, SeekCallback seekCallback, void* callbackData, AImgHandle* imgH, int32_t* detectedFileFormat)
{
    *imgH = (AImgHandle*)2;
    AImg::AImgBase** imgPtr = (AImg::AImgBase**)imgH;

    int32_t fileFormat = UNKNOWN_IMAGE_FORMAT;
    int32_t retval = AIMG_UNSUPPORTED_FILETYPE;

    for(auto it = loaders.begin(); it != loaders.end(); ++it)
    {
        if(it->second->canLoadImage(readCallback, tellCallback, seekCallback, callbackData))
        {
            fileFormat = it->second->getAImgFileFormatValue();
            *imgH = it->second->openImage(readCallback, tellCallback, seekCallback, callbackData);

            if(*imgPtr != NULL)
                retval = AImgErrorCode::AIMG_SUCCESS;
            else
                retval = AImgErrorCode::AIMG_LOAD_FAILED_EXTERNAL;

            break;
        }
    }

    if(detectedFileFormat != NULL)
        *detectedFileFormat = fileFormat;

    return retval;
}

void AImgClose(AImgHandle imgH)
{
    AImg::AImgBase* img = (AImg::AImgBase*)imgH;
    delete img;
}

int32_t AImgGetInfo(AImgHandle imgH, int32_t* width, int32_t* height, int32_t* numChannels, int32_t* bytesPerChannel, int32_t* floatOrInt, int32_t* decodedImgFormat)
{
    AImg::AImgBase* img = (AImg::AImgBase*)imgH;
    return img->getImageInfo(width, height, numChannels, bytesPerChannel, floatOrInt, decodedImgFormat);
}

int32_t AImgDecodeImage(AImgHandle imgH, void* destBuffer, int32_t forceImageFormat)
{
    AImg::AImgBase* img = (AImg::AImgBase*)imgH;
    return img->decodeImage(destBuffer, forceImageFormat);
}

void convertToRGBA32F(void* src, std::vector<float>& dest, size_t i, int32_t inFormat)
{
    switch (inFormat)
    {
        case AImgFormat::R8U:
        {
            uint8_t* srcF = ((uint8_t*)src) + (i*1);

            dest[0] = ((float)srcF[0]) / 255.0f;
            dest[1] = 0;
            dest[2] = 0;
            dest[3] = 1;

            break;
        }

        case AImgFormat::RG8U:
        {
            uint8_t* srcF = ((uint8_t*)src) + (i*2);

            dest[0] = ((float)srcF[0]) / 255.0f;
            dest[1] = ((float)srcF[1]) / 255.0f;
            dest[2] = 0;
            dest[3] = 1;

            break;
        }

        case AImgFormat::RGB8U:
        {
            uint8_t* srcF = ((uint8_t*)src) + (i*3);

            dest[0] = ((float)srcF[0]) / 255.0f;
            dest[1] = ((float)srcF[1]) / 255.0f;
            dest[2] = ((float)srcF[2]) / 255.0f;
            dest[3] = 1;

            break;
        }

        case AImgFormat::RGBA8U:
        {
            uint8_t* srcF = ((uint8_t*)src) + (i*4);

            dest[0] = ((float)srcF[0]) / 255.0f;
            dest[1] = ((float)srcF[1]) / 255.0f;
            dest[2] = ((float)srcF[2]) / 255.0f;
            dest[3] = ((float)srcF[3]) / 255.0f;

            break;
        }

        #ifdef HAVE_EXR
        case AImgFormat::R16F:
        {
            half* srcF = ((half*)src) + (i*1);

            dest[0] = (float)srcF[0];
            dest[1] = 0;
            dest[2] = 0;
            dest[3] = 1;

            break;
        }

        case AImgFormat::RG16F:
        {
            half* srcF = ((half*)src) + (i*2);

            dest[0] = (float)srcF[0];
            dest[1] = (float)srcF[1];
            dest[2] = 0;
            dest[3] = 1;

            break;
        }

        case AImgFormat::RGB16F:
        {
            half* srcF = ((half*)src) + (i*3);

            dest[0] = (float)srcF[0];
            dest[1] = (float)srcF[1];
            dest[2] = (float)srcF[2];
            dest[3] = 1;

            break;
        }

        case AImgFormat::RGBA16F:
        {
            half* srcF = ((half*)src) + (i*4);

            dest[0] = (float)srcF[0];
            dest[1] = (float)srcF[1];
            dest[2] = (float)srcF[2];
            dest[3] = (float)srcF[3];

            break;
        }
        #endif

        case AImgFormat::R16U:
        {
            uint16_t* srcF = ((uint16_t*)src) + (i*1);

            dest[0] = ((float)srcF[0]) / 65535.0f;
            dest[1] = 0;
            dest[2] = 0;
            dest[3] = 1;

            break;
        }

        case AImgFormat::RG16U:
        {
            uint16_t* srcF = ((uint16_t*)src) + (i*2);

            dest[0] = ((float)srcF[0]) / 65535.0f;
            dest[1] = ((float)srcF[1]) / 65535.0f;
            dest[2] = 0;
            dest[3] = 1;

            break;
        }

        case AImgFormat::RGB16U:
        {
            uint16_t* srcF = ((uint16_t*)src) + (i*3);

            dest[0] = ((float)srcF[0]) / 65535.0f;
            dest[1] = ((float)srcF[1]) / 65535.0f;
            dest[2] = ((float)srcF[2]) / 65535.0f;
            dest[3] = 1;

            break;
        }

        case AImgFormat::RGBA16U:
        {
            uint16_t* srcF = ((uint16_t*)src) + (i*4);

            dest[0] = ((float)srcF[0]) / 65535.0f;
            dest[1] = ((float)srcF[1]) / 65535.0f;
            dest[2] = ((float)srcF[2]) / 65535.0f;
            dest[3] = ((float)srcF[3]) / 65535.0f;

            break;
        }

        case AImgFormat::R32F:
        {
            float* srcF = ((float*)src) + (i*1);

            dest[0] = srcF[0];
            dest[1] = 0;
            dest[2] = 0;
            dest[3] = 1;

            break;
        }

        case AImgFormat::RG32F:
        {
            float* srcF = ((float*)src) + (i*2);

            dest[0] = srcF[0];
            dest[1] = srcF[1];
            dest[2] = 0;
            dest[3] = 1;

            break;
        }

        case AImgFormat::RGB32F:
        {
            float* srcF = ((float*)src) + (i*3);

            dest[0] = srcF[0];
            dest[1] = srcF[1];
            dest[2] = srcF[2];
            dest[3] = 1;

            break;
        }

        case AImgFormat::RGBA32F:
        {
            float* srcF = ((float*)src) + (i*4);

            dest[0] = srcF[0];
            dest[1] = srcF[1];
            dest[2] = srcF[2];
            dest[3] = srcF[3];

            break;
        }

        default:
        {
            break;
        }
    }
}

void convertFromRGBA32F(std::vector<float>& src, void* dst, size_t i, int32_t outFormat)
{
    switch (outFormat)
    {
        case AImgFormat::R8U:
        {
            uint8_t* dstF = ((uint8_t*)dst) + (i*1);

            dstF[0] = (uint8_t)(src[0] * 255.0f);

            break;
        }

        case AImgFormat::RG8U:
        {
            uint8_t* dstF = ((uint8_t*)dst) + (i*2);

            dstF[0] = (uint8_t)(src[0] * 255.0f);
            dstF[1] = (uint8_t)(src[1] * 255.0f);

            break;
        }

        case AImgFormat::RGB8U:
        {
            uint8_t* dstF = ((uint8_t*)dst) + (i*3);

            dstF[0] = (uint8_t)(src[0] * 255.0f);
            dstF[1] = (uint8_t)(src[1] * 255.0f);
            dstF[2] = (uint8_t)(src[2] * 255.0f);

            break;
        }

        case AImgFormat::RGBA8U:
        {
            uint8_t* dstF = ((uint8_t*)dst) + (i*4);

            dstF[0] = (uint8_t)(src[0] * 255.0f);
            dstF[1] = (uint8_t)(src[1] * 255.0f);
            dstF[2] = (uint8_t)(src[2] * 255.0f);
            dstF[3] = (uint8_t)(src[3] * 255.0f);

            break;
        }

        #ifdef HAVE_EXR
        case AImgFormat::R16F:
        {
            half* dstF = ((half*)dst) + (i*1);

            dstF[0] = src[0];

            break;
        }

        case AImgFormat::RG16F:
        {
            half* dstF = ((half*)dst) + (i*2);

            dstF[0] = src[0];
            dstF[1] = src[1];

            break;
        }

        case AImgFormat::RGB16F:
        {
            half* dstF = ((half*)dst) + (i*3);

            dstF[0] = src[0];
            dstF[1] = src[1];
            dstF[2] = src[2];

            break;
        }

        case AImgFormat::RGBA16F:
        {
            half* dstF = ((half*)dst) + (i*4);

            dstF[0] = src[0];
            dstF[1] = src[1];
            dstF[2] = src[2];
            dstF[3] = src[3];

            break;
        }
        #endif

        case AImgFormat::R16U:
        {
            uint16_t* dstF = ((uint16_t*)dst) + (i*1);

            dstF[0] = (uint16_t)(src[0] * 65535.0f);

            break;
        }

        case AImgFormat::RG16U:
        {
            uint16_t* dstF = ((uint16_t*)dst) + (i*2);

            dstF[0] = (uint16_t)(src[0] * 65535.0f);
            dstF[1] = (uint16_t)(src[1] * 65535.0f);

            break;
        }

        case AImgFormat::RGB16U:
        {
            uint16_t* dstF = ((uint16_t*)dst) + (i*3);

            dstF[0] = (uint16_t)(src[0] * 65535.0f);
            dstF[1] = (uint16_t)(src[1] * 65535.0f);
            dstF[2] = (uint16_t)(src[2] * 65535.0f);

            break;
        }

        case AImgFormat::RGBA16U:
        {
            uint16_t* dstF = ((uint16_t*)dst) + (i*4);

            dstF[0] = (uint16_t)(src[0] * 65535.0f);
            dstF[1] = (uint16_t)(src[1] * 65535.0f);
            dstF[2] = (uint16_t)(src[2] * 65535.0f);
            dstF[3] = (uint16_t)(src[3] * 65535.0f);

            break;
        }

        case AImgFormat::R32F:
        {
            float* dstF = ((float*)dst) + (i*1);

            dstF[0] = src[0];

            break;
        }

        case AImgFormat::RG32F:
        {
            float* dstF = ((float*)dst) + (i*2);

            dstF[0] = src[0];
            dstF[1] = src[1];

            break;
        }

        case AImgFormat::RGB32F:
        {
            float* dstF = ((float*)dst) + (i*3);

            dstF[0] = src[0];
            dstF[1] = src[1];
            dstF[2] = src[2];

            break;
        }

        case AImgFormat::RGBA32F:
        {
            float* dstF = ((float*)dst) + (i*4);

            dstF[0] = src[0];
            dstF[1] = src[1];
            dstF[2] = src[2];
            dstF[3] = src[3];

            break;
        }

        default:
        {
            break;
        }
    }
}

void AIGetFormatDetails(int32_t format, int32_t* numChannels, int32_t* bytesPerChannel, int32_t* floatOrInt)
{
    switch(format)
    {
        case AImgFormat::R8U:
        {
            *numChannels = 1;
            *bytesPerChannel = 1;
            *floatOrInt = AImgFloatOrIntType::FITYPE_INT;
            break;
        }

        case AImgFormat::RG8U:
        {
            *numChannels = 2;
            *bytesPerChannel = 1;
            *floatOrInt = AImgFloatOrIntType::FITYPE_INT;
            break;
        }

        case AImgFormat::RGB8U:
        {
            *numChannels = 3;
            *bytesPerChannel = 1;
            *floatOrInt = AImgFloatOrIntType::FITYPE_INT;
            break;
        }

        case AImgFormat::RGBA8U:
        {
            *numChannels = 4;
            *bytesPerChannel = 1;
            *floatOrInt = AImgFloatOrIntType::FITYPE_INT;
            break;
        }

        case AImgFormat::R16F:
        {
            *numChannels = 1;
            *bytesPerChannel = 2;
            *floatOrInt = AImgFloatOrIntType::FITYPE_FLOAT;
            break;
        }

        case AImgFormat::RG16F:
        {
            *numChannels = 2;
            *bytesPerChannel = 2;
            *floatOrInt = AImgFloatOrIntType::FITYPE_FLOAT;
            break;
        }

        case AImgFormat::RGB16F:
        {
            *numChannels = 3;
            *bytesPerChannel = 2;
            *floatOrInt = AImgFloatOrIntType::FITYPE_FLOAT;
            break;
        }

        case AImgFormat::RGBA16F:
        {
            *numChannels = 4;
            *bytesPerChannel = 2;
            *floatOrInt = AImgFloatOrIntType::FITYPE_FLOAT;
            break;
        }

        case AImgFormat::R16U:
        {
            *numChannels = 1;
            *bytesPerChannel = 2;
            *floatOrInt = AImgFloatOrIntType::FITYPE_INT;
            break;
        }

        case AImgFormat::RG16U:
        {
            *numChannels = 2;
            *bytesPerChannel = 2;
            *floatOrInt = AImgFloatOrIntType::FITYPE_INT;
            break;
        }

        case AImgFormat::RGB16U:
        {
            *numChannels = 3;
            *bytesPerChannel = 2;
            *floatOrInt = AImgFloatOrIntType::FITYPE_INT;
            break;
        }

        case AImgFormat::RGBA16U:
        {
            *numChannels = 4;
            *bytesPerChannel = 2;
            *floatOrInt = AImgFloatOrIntType::FITYPE_INT;
            break;
        }

        case AImgFormat::R32F:
        {
            *numChannels = 1;
            *bytesPerChannel = 4;
            *floatOrInt = AImgFloatOrIntType::FITYPE_FLOAT;
            break;
        }

        case AImgFormat::RG32F:
        {
            *numChannels = 2;
            *bytesPerChannel = 4;
            *floatOrInt = AImgFloatOrIntType::FITYPE_FLOAT;
            break;
        }

        case AImgFormat::RGB32F:
        {
            *numChannels = 3;
            *bytesPerChannel = 4;
            *floatOrInt = AImgFloatOrIntType::FITYPE_FLOAT;
            break;
        }

        case AImgFormat::RGBA32F:
        {
            *numChannels = 4;
            *bytesPerChannel = 4;
            *floatOrInt = AImgFloatOrIntType::FITYPE_FLOAT;
            break;
        }

        default:
        {
            *numChannels = -1;
            *bytesPerChannel = -1;
            *floatOrInt = AImgFloatOrIntType::FITYPE_UNKNOWN;
            break;
        }
    }
}

int32_t AImgConvertFormat(void* src, void* dest, int32_t width, int32_t height, int32_t inFormat, int32_t outFormat)
{
    #ifndef HAVE_EXR
    if(inFormat == AImgFormat::R16F || inFormat == AImgFormat::RG16F || inFormat == AImgFormat::RGB16F || inFormat == AImgFormat::RGBA16F ||
       outFormat == AImgFormat::R16F || outFormat == AImgFormat::RG16F || outFormat == AImgFormat::RGB16F || outFormat == AImgFormat::RGBA16F)
    {
        AISetLastErrorDetails("Bad format requested, 16 bit float formats not available when compiled without EXR support");
        return AImgErrorCode::AIMG_CONVERSION_FAILED_BAD_FORMAT;
    }
    #endif

    std::vector<float> scratch(4);

    for(int32_t i = 0; i < width*height; i++)
    {
        convertToRGBA32F(src, scratch, i, inFormat);
        convertFromRGBA32F(scratch, dest, i, outFormat);
    }

    return AImgErrorCode::AIMG_SUCCESS;
}

int32_t AImgGetWhatFormatWillBeWrittenForData(int32_t fileFormat, int32_t inputFormat)
{
    return loaders[fileFormat]->getWhatFormatWillBeWrittenForData(inputFormat);
}

int32_t AImgWriteImage(int32_t fileFormat, void* data, int32_t width, int32_t height, int32_t inputFormat, WriteCallback writeCallback,
                   TellCallback tellCallback, SeekCallback seekCallback, void* callbackData)
{
    return loaders[fileFormat]->writeImage(data, width, height, inputFormat, writeCallback, tellCallback, seekCallback, callbackData);
}

struct SimpleMemoryCallbackData
{
    int32_t size;
    uint8_t* buffer;
    int32_t currentPos;
};

int32_t CALLCONV simpleMemoryReadCallback(void* callbackData, uint8_t* dest, int32_t count)
{
    auto data = (SimpleMemoryCallbackData*)callbackData;
    
    int32_t toWrite = count;
    int32_t end = data->currentPos + count;
    
    if(end > data->size)
        toWrite = data->size - data->currentPos;

    memcpy(dest, data->buffer + data->currentPos, toWrite);

    data->currentPos += toWrite;

    return toWrite;
}

void CALLCONV simpleMemoryWriteCallback(void* callbackData, const uint8_t* src, int32_t count)
{
    auto data = (SimpleMemoryCallbackData*)callbackData;

    int32_t toWrite = count;
    int32_t end = data->currentPos + count;

    if(end > data->size)
        toWrite = data->size - data->currentPos;

    memcpy(data->buffer + data->currentPos, src, toWrite);

    data->currentPos += toWrite;
}

int32_t CALLCONV simpleMemoryTellCallback(void* callbackData)
{
    auto data = (SimpleMemoryCallbackData*)callbackData;
    return data->currentPos;
}

void CALLCONV simpleMemorySeekCallback(void* callbackData, int32_t pos)
{
    auto data = (SimpleMemoryCallbackData*)callbackData;
    data->currentPos = pos;
}

void AIGetSimpleMemoryBufferCallbacks(ReadCallback* readCallback, WriteCallback* writeCallback, TellCallback* tellCallback, SeekCallback* seekCallback, void** callbackData, void* buffer, int32_t size)
{
    *readCallback = &simpleMemoryReadCallback;
    *writeCallback = &simpleMemoryWriteCallback;
    *tellCallback = &simpleMemoryTellCallback;    
    *seekCallback = &simpleMemorySeekCallback;

    auto data = new SimpleMemoryCallbackData();
    data->size = size;
    data->buffer = (uint8_t*)buffer;
    data->currentPos = 0;

    *callbackData = data;
}

void AIDestroySimpleMemoryBufferCallbacks(ReadCallback readCallback, WriteCallback writeCallback, TellCallback tellCallback, SeekCallback seekCallback, void* callbackData)
{
    AIL_UNUSED_PARAM(readCallback);
    AIL_UNUSED_PARAM(writeCallback);
    AIL_UNUSED_PARAM(tellCallback);
    AIL_UNUSED_PARAM(seekCallback);

    auto data = (SimpleMemoryCallbackData*)callbackData;
    delete data;
}
