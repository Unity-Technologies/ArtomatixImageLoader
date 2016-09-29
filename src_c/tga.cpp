#ifdef HAVE_TGA
#include "AIL.h"
#include "tga.h"
#include "AIL_internal.h"
#include <vector>
#include <string.h>
#include <setjmp.h>
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
            return 0;
        }

        void seekCallback(void * user, int num_bytes)
        {

        }
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
        bool hasCorrectImageType = (header[2] == 0 || header[2] == 1 || header[2] == 2 || header[2] == 3 || header[2] == header[2] == 9 || header[2] == 10 || header[2] == 1);
        uint16_t paletteLength = *(uint16_t *) (header + 3);
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
