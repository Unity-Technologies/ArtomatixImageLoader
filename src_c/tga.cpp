#ifdef HAVE_TGA
#include "AIL.h"
#include "tga.h"
#include "AIL_internal.h"
#include <vector>
#include <string.h>
#include <cstring>
#include <setjmp.h>
#define STBI_ONLY_TGA
#define STB_IMAGE_IMPLEMENTATION
#include "extern/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "extern/stb_image_write.h"

namespace AImg
{

    namespace STBICallbacks
    {
        int readCallback(void * user, char * data, int size)
        {
            CallbackData * callbackFunctions = (CallbackData *)user;            
            return callbackFunctions->readCallback(callbackFunctions->callbackData, (uint8_t *)data, size);
        }

        void seekCallback(void * user, int num_bytes)
        {
            CallbackData * callbackFunctions = (CallbackData *)user;
            callbackFunctions->seekCallback(callbackFunctions->callbackData, num_bytes);            
        }

        void writeFunc(void * user, void * data, int size)
        {
            CallbackData * callbackFunctions = (CallbackData *)user;
            callbackFunctions->writeCallback(callbackFunctions->callbackData, (uint8_t *)data, size);
        }
    }

    AImgFormat getWhatFormatWillBeWrittenForDataTGA(int32_t inputFormat)
    {
        int32_t numChannels, bytesPerChannel, floatOrInt;
        AIGetFormatDetails(inputFormat, &numChannels, &bytesPerChannel, &floatOrInt);

        if(floatOrInt == AImgFloatOrIntType::FITYPE_FLOAT)
            return (AImgFormat) (AImgFormat::R8U + bytesPerChannel - 1); // convert to 8U version with same channelNum

        if(inputFormat >= AImgFormat::R8U && inputFormat <= AImgFormat::RGBA8U)
            return (AImgFormat)inputFormat;

        if(inputFormat >= AImgFormat::R16U && inputFormat <= AImgFormat::RGBA16U)
            return (AImgFormat) (AImgFormat::R8U + bytesPerChannel - 1); // convert to 8U version with same channelNum

        return AImgFormat::INVALID_FORMAT;
    }

    int32_t TGAImageLoader::initialise()
    {
        return AImgErrorCode::AIMG_SUCCESS;
    }

    bool TGAImageLoader::canLoadImage(ReadCallback readCallback, TellCallback tellCallback, SeekCallback seekCallback, void *callbackData)
    {
        uint8_t header[18];

        int startingPosition = tellCallback(callbackData);

        readCallback(callbackData, header, 18);

        seekCallback(callbackData, startingPosition);

        bool hasCorrectColourMapType = (header[1] == 0 || header[1] == 1);
        bool hasCorrectImageType = (header[2] == 0 || header[2] == 1 || header[2] == 2 || header[2] == 3 || header[2] == 9 || header[2] == 10 || header[2] == 11);
        uint16_t paletteLength = *(uint16_t *) (header + 5);
        uint16_t width = *(uint16_t *)(header + 12);
        uint16_t height = *(uint16_t *)(header + 14);

        bool correctDimensions = (width > 0 && height > 0);
        bool realBitDepth = (header[16] == 8 || header[16] == 16 || header[16] == 24 || header[16] == 32);

        bool realPalette;
        if (header[1] == 1)
            realPalette = paletteLength > 0;
        else
            realPalette = true;

        return hasCorrectColourMapType && hasCorrectImageType && realPalette && correctDimensions && realBitDepth;
    }

    std::string TGAImageLoader::getFileExtension()
    {
        return "TGA";
    }

    int32_t TGAImageLoader::getAImgFileFormatValue()
    {
        return TGA_IMAGE_FORMAT;
    }

    class TGAFile : public AImgBase
    {
        public:
            CallbackData data;
            int32_t numChannels, width, height;

            int32_t getDecodeFormat()
            {
                switch (numChannels)
                {
                    case 1:
                        return AImgFormat::R8U;
                    case 2:
                        return AImgFormat::RG8U;
                    case 3:
                        return AImgFormat::RGB8U;
                    case 4:
                        return AImgFormat::RGBA8U;
                    default:
                        return AImgFormat::INVALID_FORMAT;
                }
            }

            virtual int32_t getImageInfo(int32_t *width, int32_t *height, int32_t *numChannels, int32_t *bytesPerChannel, int32_t *floatOrInt, int32_t *decodedImgFormat, uint32_t *colorProfileLen)
            {
                *width = this->width;
                *height = this->height;
                *numChannels = this->numChannels;
                if(colorProfileLen != NULL)
                {
                    *colorProfileLen = 0;
                }

                *bytesPerChannel = 1;
                *floatOrInt = AImgFloatOrIntType::FITYPE_INT;
                *decodedImgFormat = getDecodeFormat();

                return AImgErrorCode::AIMG_SUCCESS;
            }
            
            virtual int32_t getColorProfile(char *profileName, uint8_t *colorProfile, uint32_t *colorProfileLen)
            {
                if(colorProfile != NULL)
                {
                    std::strcpy(profileName, "no_profile");
                    *colorProfileLen = 0;
                }

                return AImgErrorCode::AIMG_SUCCESS;
            }

            virtual int32_t decodeImage(void *realDestBuffer, int32_t forceImageFormat)
            {
                stbi_io_callbacks callbacks;
                callbacks.read = STBICallbacks::readCallback;
                callbacks.skip = STBICallbacks::seekCallback;

                uint8_t* loadedData = stbi_load_from_callbacks(&callbacks, &data, &width, &height, &numChannels, numChannels);

                if(!loadedData)
                {
                    mErrorDetails = "[AImg::TGAImageLoader::TGAFile::decodeImage] stbi_load_from_callbacks failed!";
                    return AImgErrorCode::AIMG_LOAD_FAILED_EXTERNAL;
                }

                int32_t decodeFormat = getDecodeFormat();
                int32_t numChannels, bytesPerChannel, floatOrInt;
                AIGetFormatDetails(decodeFormat, &numChannels, &bytesPerChannel, &floatOrInt);


                void* destBuffer = realDestBuffer;

                std::vector<uint8_t> convertTmpBuffer(0);
                if (forceImageFormat != AImgFormat::INVALID_FORMAT && forceImageFormat != decodeFormat)
                {
                    convertTmpBuffer.resize(width * height * bytesPerChannel * numChannels);
                    destBuffer = &convertTmpBuffer[0];
                }

                memcpy(destBuffer, loadedData, width * height * bytesPerChannel * numChannels);
                stbi_image_free(loadedData);


                if (forceImageFormat != AImgFormat::INVALID_FORMAT && forceImageFormat != decodeFormat)
                {
                    int32_t err = AImgConvertFormat(destBuffer, realDestBuffer, width, height, decodeFormat, forceImageFormat);
                    if(err != AImgErrorCode::AIMG_SUCCESS)
                        return err;
                }

                return AImgErrorCode::AIMG_SUCCESS;
            }

            virtual int32_t openImage(ReadCallback readCallback, TellCallback tellCallback, SeekCallback seekCallback, void *callbackData)
            {
                data.readCallback = readCallback;
                data.tellCallback = tellCallback;
                data.seekCallback = seekCallback;
                data.callbackData = callbackData;
                stbi_io_callbacks callbacks;
                callbacks.read = STBICallbacks::readCallback;
                callbacks.skip = STBICallbacks::seekCallback;

                int startingPosition = tellCallback(callbackData);
                stbi_info_from_callbacks(&callbacks, &data, &width, &height, &numChannels);
                seekCallback(callbackData, startingPosition);

                return AImgErrorCode::AIMG_SUCCESS;
            }

            virtual int32_t writeImage(void *data, int32_t width, int32_t height, int32_t inputFormat, char *profileName, uint8_t *colorProfile, uint32_t colorProfileLen,
                                    WriteCallback writeCallback, TellCallback tellCallback, SeekCallback seekCallback, void *callbackData, void* encodingOptions)
            {
                AIL_UNUSED_PARAM(encodingOptions);

                int32_t writeFormat = getWhatFormatWillBeWrittenForDataTGA(inputFormat);

                std::vector<uint8_t> convertBuffer(0);

                int32_t numChannels, bytesPerChannel, floatOrInt;
                AIGetFormatDetails(writeFormat, &numChannels, &bytesPerChannel, &floatOrInt);

                if (writeFormat != inputFormat)
                {
                    convertBuffer.resize(width * height * numChannels * bytesPerChannel);

                    int32_t convertError = AImgConvertFormat(data, &convertBuffer[0], width, height, inputFormat, writeFormat);

                    if (convertError != AImgErrorCode::AIMG_SUCCESS)
                        return convertError;
                    data = &convertBuffer[0];
                }

                CallbackData callbackFunctions;
                callbackFunctions.tellCallback = tellCallback;
                callbackFunctions.writeCallback = writeCallback;
                callbackFunctions.seekCallback = seekCallback;
                callbackFunctions.callbackData = callbackData;

                int err = stbi_write_tga_to_func(&STBICallbacks::writeFunc, &callbackFunctions, width, height, numChannels, data);
                if (err != 0)
                {
                    return AImgErrorCode::AIMG_SUCCESS;
                }
                else
                {
                    mErrorDetails = "[AImg::TGAImageLoader::TGAFile::writeImage] stbi_write_tga_to_func failed!";
                    return AImgErrorCode::AIMG_WRITE_FAILED_EXTERNAL;
                }
            }
    };

    AImgBase * TGAImageLoader::getAImg()
    {
        return new TGAFile();
    }

    AImgFormat TGAImageLoader::getWhatFormatWillBeWrittenForData(int32_t inputFormat)
    {
        return getWhatFormatWillBeWrittenForDataTGA(inputFormat);
    }

}
#endif
