#include <vector>
#include <iostream>
#include <cstring>

#include "AIL.h"
#include "AIL_internal.h"

#include "exr.h"

std::vector<AImg::ImageLoaderBase*> loaders;
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
    loaders.push_back(new AImg::ExrImageLoader());

    for(uint32_t i = 0; i < loaders.size(); i++)
    {
        int32_t err = loaders[i]->initialise();
        if(err != AImgErrorCode::AIMG_SUCCESS)
            return err;
    }

    return AImgErrorCode::AIMG_SUCCESS;
}

void AImgCleanUp()
{
    for(uint32_t i = 0; i < loaders.size(); i++)
        delete loaders[i];
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

    for(uint32_t i = 0; i < loaders.size(); ++i)
    {
        if(loaders[i]->canLoadImage(readCallback, tellCallback, seekCallback, callbackData))
        {
            fileFormat = loaders[i]->getAImgFileFormatValue();
            *imgH = loaders[i]->openImage(readCallback, tellCallback, seekCallback, callbackData);

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

void AIGetSimpleMemoryBufferCallbacks(ReadCallback* readCallback, TellCallback* tellCallback, SeekCallback* seekCallback, void** callbackData, void* buffer, int32_t size)
{
    *readCallback = &simpleMemoryReadCallback;    
    *tellCallback = &simpleMemoryTellCallback;    
    *seekCallback = &simpleMemorySeekCallback;

    auto data = new SimpleMemoryCallbackData();
    data->size = size;
    data->buffer = (uint8_t*)buffer;
    data->currentPos = 0;

    *callbackData = data;
}

void AIDestroySimpleMemoryBufferCallbacks(ReadCallback readCallback, TellCallback tellCallback, SeekCallback seekCallback, void* callbackData)
{
    AIL_UNUSED_PARAM(readCallback);
    AIL_UNUSED_PARAM(tellCallback);
    AIL_UNUSED_PARAM(seekCallback);

    auto data = (SimpleMemoryCallbackData*)callbackData;
    delete data;
}
