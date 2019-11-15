/*
 * Copyright 2016-2019 Artomatix LTD
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef ARTOMATIX_IMAGE_LOADER_BASE_H
#define ARTOMATIX_IMAGE_LOADER_BASE_H

#include <string>

#include "AIL.h"
#include "IExifHandler.hpp"
#include <memory>

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

        virtual int32_t writeImage(void* data, int32_t width, int32_t height, int32_t inputFormat, int32_t outputFormat,
            const char *profileName, uint8_t *colourProfile, uint32_t colourProfileLen,
            WriteCallback writeCallback, TellCallback tellCallback, SeekCallback seekCallback, void* callbackData, void* encodingOptions) = 0;

        const char* getErrorDetails()
        {
            return mErrorDetails.c_str();
        }

        virtual int32_t verifyEncodeOptions(void* encodeOptions)
        {
            if (encodeOptions != NULL)
            {
                mErrorDetails = "[AImgBase::verifyEncodeOptions] encode options passed to an encoder that doesn't support any options!";
                return AImgErrorCode::AIMG_INVALID_ENCODE_ARGS;
            }

            return AImgErrorCode::AIMG_SUCCESS;
        }

        virtual bool SupportsExif() const noexcept = 0;
        virtual  std::shared_ptr<IExifHandler> GetExifData(int32_t* error = nullptr) = 0;

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

        virtual bool isFormatSupported(int32_t format) = 0;

        virtual AImgFormat getWhatFormatWillBeWrittenForData(int32_t inputFormat, int32_t outputFormat) = 0;
    };
}

#endif // ARTOMATIX_IMAGE_LOADER_BASE_H
