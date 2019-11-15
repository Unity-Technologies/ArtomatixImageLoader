#include "hdr.h"
#include "AIL_internal.h"

#define STBI_ONLY_HDR

#include "extern/stb_image.h"
#include <cstring>

namespace AImg
{
    namespace STBIHDRCallbacks
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

        int eofCallback(void * user)
        {
            CallbackData * callbackFunctions = (CallbackData *)user;

            int startPos = callbackFunctions->tellCallback(callbackFunctions->callbackData);
            callbackFunctions->seekCallback(callbackFunctions->callbackData, startPos + 1);

            int newPos = callbackFunctions->tellCallback(callbackFunctions->callbackData);

            callbackFunctions->seekCallback(callbackFunctions->callbackData, startPos);

            return (startPos - newPos) != 0 ? 1 : 0;
        }
    }

    class HDRFile : public AImgBase
    {
    public:

        virtual int32_t openImage(ReadCallback readCallback, TellCallback tellCallback, SeekCallback seekCallback, void *callbackData)
        {
            data.readCallback = readCallback;
            data.tellCallback = tellCallback;
            data.seekCallback = seekCallback;
            data.callbackData = callbackData;

            stbi_io_callbacks callback;
            callback.read = STBIHDRCallbacks::readCallback;
            callback.skip = STBIHDRCallbacks::seekCallback;
            callback.eof = STBIHDRCallbacks::eofCallback;

            int startingPosition = tellCallback(callbackData);
            stbi_hdr_to_ldr_gamma(1.0f);
            stbi_ldr_to_hdr_gamma(1.0f);

            stbi_info_from_callbacks(&callback, &data, &width, &height, &numChannels);
            seekCallback(callbackData, startingPosition);

            return AImgErrorCode::AIMG_SUCCESS;
        }

        virtual int32_t decodeImage(void *realDestBuffer, int32_t forceImageFormat)
        {
            stbi_io_callbacks callbacks;

            callbacks.read = STBIHDRCallbacks::readCallback;
            callbacks.skip = STBIHDRCallbacks::seekCallback;
            callbacks.eof = STBIHDRCallbacks::eofCallback;
            float * loadedData = stbi_loadf_from_callbacks(&callbacks, &data, &width, &height, &numChannels, numChannels);

            if (!loadedData)
            {
                mErrorDetails = "[AImg::HDRImageLoader::HDRFile::decodeImage] stbi_loadf_from_callbacks failed!";
                return AImgErrorCode::AIMG_LOAD_FAILED_EXTERNAL;
            }

            int32_t decodeFormat = AImgFormat::RGB32F;
            int32_t numChannels, bytesPerChannel, floatOrInt;
            AIGetFormatDetails(decodeFormat, &numChannels, &bytesPerChannel, &floatOrInt);

            void* destBuffer = realDestBuffer;

            std::vector<uint8_t> convertTmpBuffer(0);
            if (forceImageFormat != AImgFormat::INVALID_FORMAT && forceImageFormat != decodeFormat)
            {
                convertTmpBuffer.resize(width * height * bytesPerChannel * numChannels);
                destBuffer = convertTmpBuffer.data();
            }

            memcpy(destBuffer, loadedData, width * height * bytesPerChannel * numChannels);
            stbi_image_free(loadedData);

            if (forceImageFormat != AImgFormat::INVALID_FORMAT && forceImageFormat != decodeFormat)
            {
                int32_t err = AImgConvertFormat(destBuffer, realDestBuffer, width, height, decodeFormat, forceImageFormat);
                if (err != AImgErrorCode::AIMG_SUCCESS)
                    return err;
            }

            return AImgErrorCode::AIMG_SUCCESS;
        }

        virtual int32_t writeImage(void *data, int32_t width, int32_t height, int32_t inputFormat, int32_t outputFormat, const char *profileName, uint8_t *colourProfile, uint32_t colourProfileLen,
            WriteCallback writeCallback, TellCallback tellCallback, SeekCallback seekCallback, void *callbackData, void* encodingOptions)
        {
            return AImgErrorCode::AIMG_WRITE_NOT_SUPPORTED_FOR_FORMAT;
        }

        virtual int32_t getImageInfo(int32_t* width, int32_t* height, int32_t* numChannels, int32_t* bytesPerChannel, int32_t* floatOrInt, int32_t* decodedImgFormat, uint32_t *colourProfileLen)
        {
            *width = this->width;
            *height = this->height;
            *numChannels = this->numChannels;
            *bytesPerChannel = 4;
            *floatOrInt = AImgFloatOrIntType::FITYPE_FLOAT;
            *decodedImgFormat = AImgFormat::RGB32F;

            if (colourProfileLen != NULL)
            {
                *colourProfileLen = 0;
            }
            return AImgErrorCode::AIMG_SUCCESS;
        }

        virtual int32_t getColourProfile(char *profileName, uint8_t *colourProfile, uint32_t *colourProfileLen)
        {
            if (colourProfile != NULL)
            {
                *colourProfileLen = 0;
            }
            if (profileName != NULL)
            {
                std::strcpy(profileName, "no_profile");
            }

            return AImgErrorCode::AIMG_SUCCESS;
        }

        virtual bool SupportsExif() const noexcept override
        {
            return false;
        }

        virtual std::shared_ptr<IExifHandler> GetExifData(int32_t * error) override
        {
            if (error != nullptr)
            {
                *error = AIMG_EXIF_DATA_NOT_SUPPORTED;
            }

            return std::shared_ptr<IExifHandler>();
        }

    private:
        CallbackData data;
        int32_t numChannels, width, height;
    };

    AImgBase * HDRImageLoader::getAImg()
    {
        return new HDRFile();
    }

    bool HDRImageLoader::canLoadImage(ReadCallback readCallback, TellCallback tellCallback, SeekCallback seekCallback, void* callbackData)
    {
        int startingPosition = tellCallback(callbackData);

        std::vector<uint8_t> magic = { 0x23, 0x3f, 0x52, 0x41, 0x44, 0x49, 0x41, 0x4e, 0x43, 0x45, 0x0a };

        std::vector<uint8_t> readBackData;
        readBackData.resize(magic.size());

        readCallback(callbackData, readBackData.data(), (int32_t)magic.size());

        seekCallback(callbackData, startingPosition);

        return memcmp(magic.data(), readBackData.data(), magic.size()) == 0;
    }

    std::string HDRImageLoader::getFileExtension()
    {
        return "HDR";
    }

    int32_t HDRImageLoader::getAImgFileFormatValue()
    {
        return HDR_IMAGE_FORMAT;
    }

    int32_t HDRImageLoader::initialise() { return AImgErrorCode::AIMG_SUCCESS; }

    bool HDRImageLoader::isFormatSupported(int32_t format) { return format == AImgFormat::RGB32F; }

    AImgFormat HDRImageLoader::getWhatFormatWillBeWrittenForData(int32_t inputFormat, int32_t outputFormat) { return AImgFormat::RGB32F; }
}