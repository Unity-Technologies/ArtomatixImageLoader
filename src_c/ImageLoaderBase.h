#ifndef ARTOMATIX_IMAGE_LOADER_BASE_H
#define ARTOMATIX_IMAGE_LOADER_BASE_H

#include <string>

#include "AIL.h"

namespace AImg
{
    class AImgBase
    {
        public:
            virtual ~AImgBase();

            virtual int32_t openImage(ReadCallback readCallback, TellCallback tellCallback, SeekCallback seekCallback, void* callbackData) = 0;
            virtual int32_t getImageInfo(int32_t* width, int32_t* height, int32_t* numChannels, int32_t* bytesPerChannel, int32_t* floatOrInt, int32_t* decodedImgFormat, uint32_t *colourProfileLen) = 0;
            virtual int32_t getColourProfile(char* profileName, uint8_t* colourProfile, uint32_t *colourProfileLen) = 0;
            virtual int32_t decodeImage(void* destBuffer, int32_t forceImageFormat) = 0;

            virtual int32_t writeImage(void* data, int32_t width, int32_t height, int32_t inputFormat, const char *profileName, uint8_t *colourProfile, uint32_t colourProfileLen,
                                       WriteCallback writeCallback, TellCallback tellCallback, SeekCallback seekCallback, void* callbackData, void* encodingOptions) = 0;

            const char* getErrorDetails()
            {
                return mErrorDetails.c_str();
            }

            virtual int32_t verifyEncodeOptions(void* encodeOptions)
            {
                if(encodeOptions != NULL)
                {
                    mErrorDetails = "[AImgBase::verifyEncodeOptions] encode options passed to an encoder that doesn't support any options!";
                    return AImgErrorCode::AIMG_INVALID_ENCODE_ARGS;
                }

                return AImgErrorCode::AIMG_SUCCESS;
            }

        protected:
            std::string mErrorDetails;
    };

    class ImageLoaderBase
    {
        public:
            virtual ~ImageLoaderBase();

            virtual AImgBase* getAImg() = 0;

            virtual int32_t initialise() = 0;
            virtual bool canLoadImage(ReadCallback readCallback, TellCallback tellCallback, SeekCallback seekCallback, void* callbackData) = 0;
            virtual std::string getFileExtension() = 0;
            virtual int32_t getAImgFileFormatValue() = 0;

            virtual AImgFormat getWhatFormatWillBeWrittenForData(int32_t inputFormat) = 0;
    };
}

#endif // ARTOMATIX_IMAGE_LOADER_BASE_H
