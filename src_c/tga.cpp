#ifdef HAVE_TGA
#include "AIL.h"
#include "tga.h"
#include "AIL_internal.h"
#include <vector>
#include <string.h>
#include <setjmp.h>

namespace AImg
{
    int32_t TGAImageLoader::initialise()
    {
        return AImgErrorCode::AIMG_SUCCESS;
    }

    bool TGAImageLoader::canLoadImage(ReadCallback readCallback, TellCallback tellCallback, SeekCallback seekCallback, void *callbackData)
    {
        return false;
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

            TGAFile()
            {
            }

            virtual ~TGAFile()
            {
            }

            virtual int32_t getImageInfo(int32_t *width, int32_t *height, int32_t *numChannels, int32_t *bytesPerChannel, int32_t *floatOrInt, int32_t *decodedImgFormat)
            {

                return AImgErrorCode::AIMG_SUCCESS;
            }

            virtual int32_t decodeImage(void *destBuffer, int32_t forceImageFormat)
            {
                return AImgErrorCode::AIMG_SUCCESS;
            }
    };


    AImgBase* TGAImageLoader::openImage(ReadCallback readCallback, TellCallback tellCallback, SeekCallback seekCallback, void *callbackData)
    {
        TGAFile* tga = new TGAFile();
        return tga;
    }

    AImgFormat TGAImageLoader::getWhatFormatWillBeWrittenForData(int32_t inputFormat)
    {
        AIL_UNUSED_PARAM(inputFormat);
        return AImgFormat::INVALID_FORMAT;
    }

    int32_t TGAImageLoader::writeImage(void *data, int32_t width, int32_t height, int32_t inputFormat, WriteCallback writeCallback, TellCallback tellCallback, SeekCallback seekCallback, void *callbackData)
    {
        return AImgErrorCode::AIMG_SUCCESS;
    }
}
#endif
