#ifdef HAVE_TIFF

#include <stddef.h>
#include <tiffio.h>
#include <vector>
#include <string.h>
#include <math.h>
#include <iostream>
#include <algorithm>

#include "AIL.h"
#include "AIL_internal.h"
#include "tiff.h"

namespace AImg
{
    AImgFormat getWriteFormatTiff(int32_t inputFormat)
    {
        // Tiff can write all currently supported formats. Here we are just future-proofing in case we add some more formats later that tiff can't do
        if(inputFormat > AImgFormat::INVALID_FORMAT && inputFormat <= AImgFormat::RGBA32F)
            return (AImgFormat)inputFormat;

        return AImgFormat::INVALID_FORMAT;
    }

    struct tiffCallbackData
    {
        ReadCallback mReadCallback = nullptr;
        WriteCallback mWriteCallback = nullptr;
        TellCallback mTellCallback = nullptr;
        SeekCallback mSeekCallback = nullptr;
        void* callbackData = nullptr;

        int32_t startPos = 0;
        int32_t furthestPositionWritten = 0;
    };

    tsize_t tiffRead(thandle_t st, tdata_t buffer, tsize_t size)
    {
        tiffCallbackData* callbacks = (tiffCallbackData*)st;

        return callbacks->mReadCallback(callbacks->callbackData, (uint8_t*)buffer, size);
    }

    tsize_t tiff_Write(thandle_t st, tdata_t buffer, tsize_t size)
    {
        tiffCallbackData* callbacks = (tiffCallbackData*)st;

        int32_t start = callbacks->mTellCallback(callbacks->callbackData);
        callbacks->mWriteCallback(callbacks->callbackData, (uint8_t*)buffer, size);
        int32_t end = callbacks->mTellCallback(callbacks->callbackData);

        if(end > callbacks->furthestPositionWritten)
            callbacks->furthestPositionWritten = end;

        return end - start;
    }

    // This should never be called
    // We don't implement it because AImg is designed not to have the file size
    // available to it, we just receive streams. I had a look at the libtiff source,
    // and it seems to only be used for a deprecated version of embedded jpeg tiffs,
    // and tiff directories. We just cancel decode if we detect an old-style jpeg
    // embedded tiff, and never use the directory crap in the first place, so it
    // should be safe to just return 0 here.
    toff_t tiff_Size(thandle_t st)
    {
        AIL_UNUSED_PARAM(st);
        return 0;
    }

    toff_t tiff_Seek(thandle_t st, toff_t pos, int whence)
    {
        tiffCallbackData* callbacks = (tiffCallbackData*)st;

        if (pos == 0xFFFFFFFF) return 0xFFFFFFFF;

        toff_t finalPos = pos;

        switch (whence)
        {
            case SEEK_SET:
            {
                finalPos += callbacks->startPos;
                break;
            }

            case SEEK_CUR:
            {
                finalPos += callbacks->mTellCallback(callbacks->callbackData);
                break;
            }

            // I checked the libtiff source, this should only be used during image
            // writing. In this case, we can keep track of the file size, because we are
            // setting the size by writing to the stream, so we can actually implement it,
            // unlike tiff_Size above.
            case SEEK_END:
            {
                finalPos = callbacks->furthestPositionWritten + pos;
                break;
            }
        }

        callbacks->mSeekCallback(callbacks->callbackData, finalPos);

        return callbacks->mTellCallback(callbacks->callbackData);
    }

    int tiff_Map(thandle_t, tdata_t*, toff_t*)
    {
        return 0;
    }

    void tiff_Unmap(thandle_t, tdata_t, toff_t)
    {
        return;
    }

    int tiff_Close(thandle_t)
    {
        return 0;
    }

    float convertFloat24(const unsigned char* src)
    {
        int mantissaBits = 16;
        int exponentBits = 7;
        int maxExponent = pow(2, exponentBits) - 1;

        int v = *((int*)src);
        int sign = v >> 23;
        int exponent = (v >> mantissaBits) & (int)(pow(2, exponentBits) - 1);
        int mantissa = v & (int)(pow(2, mantissaBits) - 1);

        if (exponent == 0)
        {
            if (mantissa != 0)
            {
                while ((mantissa & (int)pow(2, mantissaBits)) == 0)
                {
                    mantissa <<= 1;
                    exponent--;
                }

                exponent++;
                mantissa &= (int)(pow(2, mantissaBits) - 1);
                exponent += 127 - (pow(2, exponentBits - 1) - 1);
            }
        }
        else if (exponent == maxExponent)
        {
            exponent = 255;
        }
        else
        {
            exponent += 127 - (pow(2, exponentBits - 1) - 1);
        }

        mantissa <<= (23 - mantissaBits);

        int value = (sign << 31) | (exponent << 23) | mantissa;

        float final = *((float*)&value);

        return final;
    }

    class TiffFile : public AImgBase
    {
        TIFF* tiff = nullptr;
        tiffCallbackData callbacks;

        uint16_t bitsPerChannel = 0;
        uint16_t channels = 0;
        uint32_t width = 0, height = 0;
        uint16_t sampleFormat = 0;
        uint16_t compression = 0;
        uint32_t rowsPerStrip = 0;
        uint16_t planarConfig = 0;


        public:

            virtual ~TiffFile()
            {
                TIFFClose(tiff);
            }

            int32_t getDecodeFormat()
            {
                if(channels > 0 && channels <= 4)
                {
                    // handle 24-bit float (lolwtf)
                    if(bitsPerChannel == 24 && sampleFormat == SAMPLEFORMAT_IEEEFP)
                        return ((int32_t)AImgFormat::R32F) - 1 + channels;

                    if(sampleFormat == SAMPLEFORMAT_IEEEFP)
                    {
                        if(bitsPerChannel == 16)
                            return ((int32_t)AImgFormat::R16F) - 1 + channels;
                        else if(bitsPerChannel == 32)
                            return ((int32_t)AImgFormat::R32F) - 1 + channels;
                    }
                    else if(sampleFormat == SAMPLEFORMAT_UINT || sampleFormat == SAMPLEFORMAT_INT)
                    {
                        if(bitsPerChannel == 8)
                            return ((int32_t)AImgFormat::R8U)  - 1 + channels;
                        else if(bitsPerChannel == 16)
                            return ((int32_t)AImgFormat::R16U) - 1 + channels;
                    }
                }

                return AImgFormat::INVALID_FORMAT;
            }

            virtual int32_t getImageInfo(int32_t* width, int32_t* height, int32_t* numChannels, int32_t* bytesPerChannel, int32_t* floatOrInt, int32_t* decodedImgFormat)
            {
                *width = this->width;
                *height = this->height;
                *numChannels = this->channels;

                if(bitsPerChannel % 8 == 0)
                    *bytesPerChannel = bitsPerChannel / 8;
                else
                    *bytesPerChannel = -1;

                if(sampleFormat == SAMPLEFORMAT_IEEEFP)
                    *floatOrInt = AImgFloatOrIntType::FITYPE_FLOAT;
                else if(sampleFormat == SAMPLEFORMAT_UINT || sampleFormat == SAMPLEFORMAT_INT)
                    *floatOrInt = AImgFloatOrIntType::FITYPE_INT;
                else
                    *floatOrInt = AImgFloatOrIntType::FITYPE_UNKNOWN;

                *decodedImgFormat = getDecodeFormat();

                return AImgErrorCode::AIMG_SUCCESS;
            }

            virtual int32_t decodeImage(void* realDestBuffer, int32_t forceImageFormat)
            {
                uint8_t* destBuffer = (uint8_t*)realDestBuffer;


                int32_t decodeFormat = getDecodeFormat();

                std::vector<uint8_t> convertTmpBuffer(0);
                if (forceImageFormat != AImgFormat::INVALID_FORMAT && forceImageFormat != decodeFormat)
                {
                    int32_t numChannels, bytesPerChannelF, floatOrInt;
                    AIGetFormatDetails(decodeFormat, &numChannels, &bytesPerChannelF, &floatOrInt);

                    convertTmpBuffer.resize(width * height * bytesPerChannelF * numChannels);
                    destBuffer = &convertTmpBuffer[0];
                }

                uint32 stripsize = TIFFStripSize(tiff);
                int32_t bytesPerChannel = bitsPerChannel / 8;

                std::vector<char> stripBuffer(stripsize);

                int32_t _;
                int32_t decodeFormatBytesPerChannel;
                AIGetFormatDetails(decodeFormat, &_, &decodeFormatBytesPerChannel, &_);

                if (planarConfig == PLANARCONFIG_CONTIG)
                {
                    unsigned char* bufferPtr = destBuffer;

                    size_t row = 0;
                    for (tstrip_t strip = 0; strip < TIFFNumberOfStrips(tiff); strip++)
                    {
                        if(TIFFReadEncodedStrip(tiff, strip, &stripBuffer[0], -1) == ((tmsize_t)-1)) // this function returns -1 on failure. As an unsigned int. yaaaaaaaaaaay
                        {
                            mErrorDetails = "[AImg::TIFFImageLoader::TiffFile::openImage] Tiff read failure, TIFFReadEncodedStrip failed";
                            return AImgErrorCode::AIMG_LOAD_FAILED_EXTERNAL;
                        }

                        char* stripPtr = &stripBuffer[0];

                        for (size_t rowStrip = 0; rowStrip < rowsPerStrip; rowStrip++)
                        {
                            if (row >= height)
                                break;

                            for (size_t x = 0; x < width; x++)
                            {
                                for (size_t channelIndex = 0; channelIndex < channels; channelIndex++)
                                {
                                    if (bytesPerChannel == 4)
                                    {
                                        *((float*)bufferPtr) = *(float*)stripPtr;
                                        stripPtr += 4;
                                        bufferPtr += 4;
                                    }
                                    else if (bytesPerChannel == 3)
                                    {
                                        // this will always be 24-bit float, as we return an error in openImage if BITSPERSAMPLE == 3 and SAMPLEFORMAT is not IEEEFP
                                        *((float*)bufferPtr) = convertFloat24((unsigned char*)stripPtr);
                                        stripPtr += 3;
                                        bufferPtr += 4;
                                    }
                                    else if (bytesPerChannel == 2)
                                    {
                                        // doesn't matter if we have 16-bit int or float, we can just copy the data over all the same
                                        *((uint16_t*)bufferPtr) = *((uint16_t*)stripPtr);

                                        stripPtr += 2;
                                        bufferPtr += 2;
                                    }
                                    else if (bytesPerChannel == 1)
                                    {
                                        *bufferPtr = *stripPtr;
                                        stripPtr += 1;
                                        bufferPtr += 1;
                                    }
                                }

                                // convert from YCbCr to RGB (jpeg tiffs always have bytesPerChannel == 1 and PLANARCONFIG_CONTIG, and always have three channels)
                                if (compression == COMPRESSION_JPEG)
                                {
                                    float Y = bufferPtr[-3];
                                    float Cb = bufferPtr[-2];
                                    float Cr = bufferPtr[-1];

                                    bufferPtr[-3] = (char)std::max(std::min(Y + 1.40200 * (Cr - 127.0), 255.0), 0.0);
                                    bufferPtr[-2] = (char)std::max(std::min(Y - 0.34414 * (Cb - 127.0) - 0.71414 * (Cr - 127.0), 255.0), 0.0);
                                    bufferPtr[-1] = (char)std::max(std::min(Y + 1.77200 * (Cb - 127.0), 255.0), 0.0);
                                }
                            }
                            row++;
                        }
                    }
                }

                // This is just a copy of the above block, fixed up to work for channels being stored sequentially not interleaved.
                // for clarity, interleaved for a 2x1 image would be: R1,G1,B1,R2,G2,B2, where as SEPARATE would be R1,R2,G1,G2,B1,B2
                // We don't actually support decoding separated channel buffers like that, so we manually interleave the channels to fix it up.
                else if (planarConfig == PLANARCONFIG_SEPARATE)
                {
                    int32_t channelIndex = -1;
                    unsigned char* bufferPtr = NULL;

                    while (true)
                    {
                        size_t row = 0;
                        for (tstrip_t strip = 0; strip < TIFFNumberOfStrips(tiff); strip++)
                        {
                            if(TIFFReadEncodedStrip(tiff, strip, &stripBuffer[0], -1) == ((tmsize_t)-1))
                            {
                                mErrorDetails = "[AImg::TIFFImageLoader::TiffFile::openImage] Tiff read failure, TIFFReadEncodedStrip failed";
                                return AImgErrorCode::AIMG_LOAD_FAILED_EXTERNAL;
                            }

                            char* stripPtr = (char*)&stripBuffer[0];

                            for (size_t rowStrip = 0; rowStrip < rowsPerStrip; rowStrip++)
                            {
                                if (row % height == 0)
                                {
                                    channelIndex++;
                                    if (channelIndex >= channels)
                                        goto done;

                                    bufferPtr = destBuffer + channelIndex*decodeFormatBytesPerChannel;
                                }

                                for (size_t x = 0; x < width; x++)
                                {
                                    if (bytesPerChannel == 4)
                                    {
                                        *((float*)bufferPtr) = *(float*)stripPtr;
                                        stripPtr += 4;
                                        bufferPtr += 4 * channels;
                                    }
                                    else if (bytesPerChannel == 3)
                                    {
                                        *((float*)bufferPtr) = convertFloat24((unsigned char*)stripPtr);
                                        stripPtr += 3;
                                        bufferPtr += 4 * channels;
                                    }
                                    else if (bytesPerChannel == 2)
                                    {
                                        *((uint16_t*)bufferPtr) = *((uint16*)stripPtr);

                                        stripPtr += 2;
                                        bufferPtr += 2 * channels;
                                    }
                                    else if (bytesPerChannel == 1)
                                    {
                                        *bufferPtr = *stripPtr;
                                        stripPtr += 1;
                                        bufferPtr += 1 * channels;
                                    }
                                }
                                row++;
                            }
                        }
                    }
                    done:;
                }

                if (forceImageFormat != AImgFormat::INVALID_FORMAT && forceImageFormat != decodeFormat)
                {
                    int32_t err = AImgConvertFormat(destBuffer, realDestBuffer, width, height, decodeFormat, forceImageFormat);
                    if(err != AImgErrorCode::AIMG_SUCCESS)
                        return err;
                }


                return AImgErrorCode::AIMG_SUCCESS;
            }

            virtual int32_t openImage(ReadCallback readCallback, TellCallback tellCallback, SeekCallback seekCallback, void* callbackData)
            {
                callbacks.mReadCallback = readCallback;
                callbacks.mSeekCallback = seekCallback;
                callbacks.mTellCallback = tellCallback;
                callbacks.callbackData = callbackData;
                callbacks.startPos = tellCallback(callbackData);

                tiff = TIFFClientOpen("", "r", (thandle_t)&callbacks, tiffRead, tiff_Write, tiff_Seek, tiff_Close, tiff_Size, tiff_Map, tiff_Unmap);

                if(tiff == nullptr)
                {
                    mErrorDetails = "[AImg::TIFFImageLoader::TiffFile::openImage] Failed to open tiff file for unknown reason";
                    return AImgErrorCode::AIMG_LOAD_FAILED_EXTERNAL;
                }

                bool hasEssentialTiffTags = TIFFGetField(tiff, TIFFTAG_SAMPLESPERPIXEL, &channels);

                AImgErrorCode retval = AImgErrorCode::AIMG_SUCCESS;

                if(channels > 0 && channels <= 4)
                {

                    int16_t bpsNotRead = -999;
                    std::vector<int16_t> bitsPerSampleValues(channels, bpsNotRead);

                    uint32_t* stripByteCounts = NULL;

                    hasEssentialTiffTags = hasEssentialTiffTags &&
                                           TIFFGetField(tiff, TIFFTAG_BITSPERSAMPLE, &bitsPerSampleValues[0]) &&
                                           TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &width) &&
                                           TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &height) &&
                                           TIFFGetField(tiff, TIFFTAG_COMPRESSION, &compression) &&
                                           TIFFGetField(tiff, TIFFTAG_ROWSPERSTRIP, &rowsPerStrip) &&
                                           TIFFGetField(tiff, TIFFTAG_PLANARCONFIG, &planarConfig) &&
                                           TIFFGetField(tiff, TIFFTAG_STRIPBYTECOUNTS, &stripByteCounts);


                    if(hasEssentialTiffTags)
                    {
                        bitsPerChannel = bitsPerSampleValues[0];

                        for(size_t i = 0; i < bitsPerSampleValues.size(); i++)
                        {
                            // check if they're all the same, if they have been read at all
                            // tiff docs are inconsistent on this, http://www.awaresystems.be/imaging/tiff/tifftags/bitspersample.html says it should return
                            // one value for each channel, http://libtiff.maptools.org/man/TIFFGetField.3tiff.html says just one. (retrieved 10/21/2016)
                            // I try to work for both (in my experience though, it's just one).
                            if(bitsPerSampleValues[i] != bitsPerSampleValues[0] && bitsPerSampleValues[i] != bpsNotRead)
                            {
                                mErrorDetails = "Bits per sample is not the same for all channels.";
                                retval = AImgErrorCode::AIMG_LOAD_FAILED_UNSUPPORTED_TIFF;
                            }
                        }
                    }
                }
                else
                {
                    mErrorDetails = "Channel count " + std::to_string(channels) + " we only support up to 4";
                    retval = AImgErrorCode::AIMG_LOAD_FAILED_UNSUPPORTED_TIFF;
                }

                if(!hasEssentialTiffTags)
                {
                    mErrorDetails = "[AImg::TIFFImageLoader::TiffFile::openImage] Bad tiff file - missing at least one of the essential tifftags "
                                    "(BITSPERSAMPLE, SAMPLESPERPIXEL, IMAGEWIDTH, IMAGELENGTH, COMPRESSION, ROWSPERSTRIP, PLANARCONFIG, STRIPBYTECOUNTS)";
                    return AImgErrorCode::AIMG_LOAD_FAILED_INTERNAL;
                }

                if (!TIFFGetField(tiff, TIFFTAG_SAMPLEFORMAT, &sampleFormat))
                    sampleFormat = SAMPLEFORMAT_UINT; // default to uint format if no SAMPLEFORMAT tifftag is present

                if(compression == COMPRESSION_OJPEG)
                {
                    mErrorDetails = "Old-style jpeg tiff detected.";
                    retval = AImgErrorCode::AIMG_LOAD_FAILED_UNSUPPORTED_TIFF;
                }

                if(compression == COMPRESSION_JPEG)
                {
                    mErrorDetails = "[AImg::TIFFImageLoader::TiffFile::openImage] Jpeg compressed tiff not currently supported. Will be added in a future version.";
                    return AImgErrorCode::AIMG_LOAD_FAILED_INTERNAL;
                }

                if(bitsPerChannel % 8 != 0)
                {
                    mErrorDetails = "Bits per channel is not divisible by 8.";
                    retval =  AImgErrorCode::AIMG_LOAD_FAILED_UNSUPPORTED_TIFF;
                }

                if(getDecodeFormat() == AImgFormat::INVALID_FORMAT)
                {
                    mErrorDetails = "Unsupported combination of tifftags BITSPERSAMPLE and SAMPLEFORMAT.";
                    retval = AIMG_LOAD_FAILED_UNSUPPORTED_TIFF;
                }


                if(retval != AImgErrorCode::AIMG_SUCCESS)
                {
                    mErrorDetails = "[AImg::TIFFImageLoader::TiffFile::openImage] " +
                                    mErrorDetails +
                                    " Only a sensible subset of tiffs are supported, this file is "
                                    "outside that subset.";
                }

                return retval;
            }

            int32_t writeImage(void* data, int32_t width, int32_t height, int32_t inputFormat, WriteCallback writeCallback,
                               TellCallback tellCallback, SeekCallback seekCallback, void* callbackData)
            {                
                tiffCallbackData wCallbacks;
                wCallbacks.mWriteCallback = writeCallback;
                wCallbacks.mSeekCallback = seekCallback;
                wCallbacks.mTellCallback = tellCallback;
                wCallbacks.callbackData = callbackData;
                wCallbacks.startPos = tellCallback(callbackData);
                TIFF* wTiff = TIFFClientOpen("", "w", (thandle_t)&wCallbacks, tiffRead, tiff_Write, tiff_Seek, tiff_Close, tiff_Size, tiff_Map, tiff_Unmap);

                int32_t retval = AIMG_SUCCESS;

                int32_t wFormat = getWriteFormatTiff(inputFormat);
                if(wFormat == AImgFormat::INVALID_FORMAT)
                {
                    mErrorDetails = "[AImg::TIFFImageLoader::TiffFile::writeImage] Cannot write this format to tiff."; // developers: see comment in getWriteFormatTiff
                    retval = AImgErrorCode::AIMG_WRITE_FAILED_INTERNAL;
                }
                else
                {
                    int32_t numChannels, bytesPerChannel, floatOrInt;
                    AIGetFormatDetails(wFormat, &numChannels, &bytesPerChannel, &floatOrInt);

                    TIFFSetField(wTiff, TIFFTAG_IMAGEWIDTH, width);
                    TIFFSetField(wTiff, TIFFTAG_IMAGELENGTH, height);
                    TIFFSetField(wTiff, TIFFTAG_SAMPLESPERPIXEL, numChannels);
                    TIFFSetField(wTiff, TIFFTAG_BITSPERSAMPLE, bytesPerChannel * 8);
                    TIFFSetField(wTiff, TIFFTAG_SAMPLEFORMAT, floatOrInt == AImgFloatOrIntType::FITYPE_FLOAT ? SAMPLEFORMAT_IEEEFP : SAMPLEFORMAT_UINT);
                    TIFFSetField(wTiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
                    TIFFSetField(wTiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
                    TIFFSetField(wTiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);

                    tsize_t stripRows = TIFFDefaultStripSize(wTiff, 0);

                    TIFFSetField(wTiff, TIFFTAG_ROWSPERSTRIP, stripRows);

                    for(int32_t y = 0; y < height; y++)
                    {
                        if(TIFFWriteScanline(wTiff, &((uint8_t*)data)[numChannels * bytesPerChannel * width * y], y, 0) < 0)
                        {
                            mErrorDetails = "[AImg::TIFFImageLoader::TiffFile::writeImage] TIFFWriteScanline failed.";
                            retval = AImgErrorCode::AIMG_WRITE_FAILED_EXTERNAL;
                            break;
                        }
                    }
                }

                TIFFClose(wTiff);

                // Leave the pointer at the end of the file, because libtiff doesn't... because it's a fantastic piece of software
                wCallbacks.mSeekCallback(wCallbacks.callbackData, wCallbacks.furthestPositionWritten);

                return retval;
            }

    };


    AImgBase* TIFFImageLoader::getAImg()
    {
        return new TiffFile();
    }

    int32_t TIFFImageLoader::initialise()
    {
        return AImgErrorCode::AIMG_SUCCESS;
    }

    bool TIFFImageLoader::canLoadImage(ReadCallback readCallback, TellCallback tellCallback, SeekCallback seekCallback, void* callbackData)
    {
        int32_t startingPos = tellCallback(callbackData);

        std::vector<uint8_t> header(4);
        readCallback(callbackData, &header[0], 4);

        seekCallback(callbackData, startingPos);

        return (header[0] == 0x49 && header[1] == 0x49 && header[2] == 0x2a && header[3] == 0x00) ||
               (header[0] == 0x4d && header[1] == 0x4d && header[2] == 0x00 && header[3] == 0x2a);
    }

    std::string TIFFImageLoader::getFileExtension()
    {
        return "tiff";
    }

    int32_t TIFFImageLoader::getAImgFileFormatValue()
    {
        return AImgFileFormat::TIFF_IMAGE_FORMAT;
    }

    AImgFormat TIFFImageLoader::getWhatFormatWillBeWrittenForData(int32_t inputFormat)
    {
        return getWriteFormatTiff(inputFormat);
    }
}

#endif // HAVE_TIFF
