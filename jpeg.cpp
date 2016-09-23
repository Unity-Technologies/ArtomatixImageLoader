#include "AIL.h"
#include "jpeg.h"
#include "AIL_internal.h"
#include <vector>
#include <jpeglib.h>

namespace AImg
{
    int32_t JPEGImageLoader::initialise()
    {
        return AImgErrorCode::AIMG_SUCCESS;
    }

    bool JPEGImageLoader::canLoadImage(ReadCallback readCallback, TellCallback tellCallback, SeekCallback seekCallback, void *callbackData)
    {
        return false;
    }

    std::string JPEGImageLoader::getFileExtension()
    {
        return "JPEG";
    }

    int32_t JPEGImageLoader::getAImgFileFormatValue()
    {
        return JPEG_IMAGE_FORMAT;
    }

    class JPEGFile : public AImgBase
    {
        public:

            virtual int32_t getImageInfo(int32_t *width, int32_t *height, int32_t *numChannels, int32_t *bytesPerChannel, int32_t *floatOrInt, int32_t *decodedImgFormat)
            {

                return AImgErrorCode::AIMG_SUCCESS;
            }

            int32_t getDecodeFormat()
            {

                return AImgFormat::INVALID_FORMAT;

            }

            virtual int32_t decodeImage(void *destBuffer, int32_t forceImageFormat)
            {
                return AImgErrorCode::AIMG_SUCCESS;
            }
    };


    AImgBase* JPEGImageLoader::openImage(ReadCallback readCallback, TellCallback tellCallback, SeekCallback seekCallback, void *callbackData)
    {
        JPEGFile* jpeg = new JPEGFile();


        return jpeg;
    }

    AImgFormat JPEGImageLoader::getWhatFormatWillBeWrittenForData(int32_t inputFormat)
    {
        return AImgFormat::INVALID_FORMAT;
    }

    int32_t JPEGImageLoader::writeImage(void *data, int32_t width, int32_t height, int32_t inputFormat, WriteCallback writeCallback, TellCallback tellCallback, SeekCallback seekCallback, void *callbackData)
    {
        return AImgErrorCode::AIMG_SUCCESS;
    }
}
