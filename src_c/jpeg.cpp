#include "AIL.h"
#include "jpeg.h"
#include "AIL_internal.h"
#include <vector>
#include <string.h>
#include <setjmp.h>
#include <jpeglib.h>

#ifdef HAVE_JPEG
namespace AImg
{
    typedef struct
    {
        jpeg_source_mgr pub;
        void *data;
        CallbackData callbackFunctionData;

    } ArtomatixJPEGSourceMGR;

    typedef struct
    {
        jpeg_destination_mgr pub;
        void *buffer;
        CallbackData callbackFunctionData;
    } ArtomatixJPEGDestinationMGR;

    typedef struct
    {
            jpeg_error_mgr pub;
            jmp_buf buf;
    } ArtomatixErrorStruct;

    namespace JPEGConsts
    {
        // Buffer size used in libjpeg
        const size_t BUFFER_SIZE = 4096;
        const int Quality = 99;

    }

    namespace JPEGCallbackFunctions
    {
        namespace WriteFunctions
        {
            void initDestination(j_compress_ptr cinfo)
            {
                AIL_UNUSED_PARAM(cinfo);
            }

            boolean emptyOutputBuffer(j_compress_ptr cinfo)
            {
                ArtomatixJPEGDestinationMGR * dst = (ArtomatixJPEGDestinationMGR *) cinfo->dest;

                dst->callbackFunctionData.writeCallback(dst->callbackFunctionData.callbackData, (uint8_t *)dst->buffer, JPEGConsts::BUFFER_SIZE);
                dst->pub.next_output_byte = (JOCTET *) dst->buffer;
                dst->pub.free_in_buffer = JPEGConsts::BUFFER_SIZE;
                return TRUE;
            }

            void termDestination(j_compress_ptr cinfo)
            {

                ArtomatixJPEGDestinationMGR * dst = (ArtomatixJPEGDestinationMGR *) cinfo->dest;
                size_t datacount = JPEGConsts::BUFFER_SIZE - dst->pub.free_in_buffer;
                if (datacount > 0)
                    dst->callbackFunctionData.writeCallback(dst->callbackFunctionData.callbackData, (uint8_t *)dst->buffer, datacount);
            }
        }

        namespace ReadFunctions
        {
            void initSource(j_decompress_ptr cinfo)
            {
                AIL_UNUSED_PARAM(cinfo);
            }

            void skipInputData(j_decompress_ptr cinfo, long num_bytes)
            {
                ArtomatixJPEGSourceMGR * src = (ArtomatixJPEGSourceMGR *)cinfo->src;
                if (num_bytes > 0)
                {
                    while (num_bytes > (long)src->pub.bytes_in_buffer)
                    {

                        num_bytes -= src->pub.bytes_in_buffer;
                        (*src->pub.fill_input_buffer)(cinfo);
                    }

                    src->pub.next_input_byte += (size_t) num_bytes;
                    src->pub.bytes_in_buffer -= (size_t) num_bytes;
                }
            }

            boolean fillInputBuffer(j_decompress_ptr cinfo)
            {
                ArtomatixJPEGSourceMGR * src  = (ArtomatixJPEGSourceMGR *)cinfo->src;
                size_t bytesRead = src->callbackFunctionData.readCallback(src->callbackFunctionData.callbackData, (uint8_t *)src->data, JPEGConsts::BUFFER_SIZE);

                if (bytesRead <= 0)
                    return FALSE;

                src->pub.bytes_in_buffer = bytesRead;
                src->pub.next_input_byte = (JOCTET *)src->data;

                return TRUE;
            }

            void termSource(j_decompress_ptr cinfo)
            {
                AIL_UNUSED_PARAM(cinfo);
            }
        }

        void lessAnnoyingEmitMessage(j_common_ptr cinfo, int msg_level)
        {
              if (msg_level == 0)
                (*cinfo->err->output_message) (cinfo);
        }

        void handleFatalError(j_common_ptr cinfo)
        {
            ArtomatixErrorStruct * err = (ArtomatixErrorStruct *) cinfo->err;
            longjmp(err->buf, 1);
        }
    }

    void setArtomatixSourceMGR(j_decompress_ptr cinfo, CallbackData callbackData)
    {
        if (cinfo->src == NULL)
        {
            cinfo->src = (jpeg_source_mgr *)(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_PERMANENT, sizeof(ArtomatixJPEGSourceMGR));
            ((ArtomatixJPEGSourceMGR * )cinfo->src)->data = (void *)(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_PERMANENT, JPEGConsts::BUFFER_SIZE);
            ((ArtomatixJPEGSourceMGR *)cinfo->src)->callbackFunctionData = callbackData;
        }

        ArtomatixJPEGSourceMGR * src  = (ArtomatixJPEGSourceMGR *)cinfo->src;
        src->pub.init_source = JPEGCallbackFunctions::ReadFunctions::initSource;
        src->pub.fill_input_buffer = JPEGCallbackFunctions::ReadFunctions::fillInputBuffer;
        src->pub.skip_input_data = JPEGCallbackFunctions::ReadFunctions::skipInputData;
        src->pub.term_source = JPEGCallbackFunctions::ReadFunctions::termSource;
        src->pub.resync_to_restart = jpeg_resync_to_restart; // Default function from libjpeg
        src->pub.next_input_byte = (JOCTET *)src->data;
        src->pub.bytes_in_buffer = 0;
    }


    void setArtomatixDestinationMGR(j_compress_ptr cinfo, CallbackData callbackData)
    {
        if (cinfo->dest == NULL)
        {
            cinfo->dest = (jpeg_destination_mgr *)(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_PERMANENT, sizeof(ArtomatixJPEGDestinationMGR));
            ((ArtomatixJPEGDestinationMGR *)cinfo->dest)->buffer = (void *)(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_PERMANENT, JPEGConsts::BUFFER_SIZE);
        }

        ArtomatixJPEGDestinationMGR * src = (ArtomatixJPEGDestinationMGR *)cinfo->dest;
        src->callbackFunctionData = callbackData;
        src->pub.init_destination = JPEGCallbackFunctions::WriteFunctions::initDestination;
        src->pub.empty_output_buffer = JPEGCallbackFunctions::WriteFunctions::emptyOutputBuffer;
        src->pub.term_destination = JPEGCallbackFunctions::WriteFunctions::termDestination;
        src->pub.next_output_byte = (JOCTET *) src->buffer;
        src->pub.free_in_buffer = JPEGConsts::BUFFER_SIZE;

    }

    int32_t JPEGImageLoader::initialise()
    {
        return AImgErrorCode::AIMG_SUCCESS;
    }

    bool JPEGImageLoader::canLoadImage(ReadCallback readCallback, TellCallback tellCallback, SeekCallback seekCallback, void *callbackData)
    {
        AIL_UNUSED_PARAM(tellCallback);
        uint8_t magic[] = {0xFF, 0xD8, 0xFF};

        int startingPosition = tellCallback(callbackData);
        std::vector<uint8_t> header(4);        
        readCallback(callbackData, &header[0], 4);

        seekCallback(callbackData, startingPosition);

        return ((int32_t)memcmp(&header[0], magic, 3)) == 0;
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
            jpeg_decompress_struct jpeg_read_struct;
            ArtomatixErrorStruct err_mgr;


            JPEGFile()
            {
                jpeg_create_decompress(&jpeg_read_struct);
            }

            virtual ~JPEGFile()
            {
                jpeg_destroy_decompress(&jpeg_read_struct);
            }


            int32_t openImage(ReadCallback readCallback, TellCallback tellCallback, SeekCallback seekCallback, void *callbackData)
            {
                CallbackData data;
                data.callbackData = callbackData;
                data.readCallback = readCallback;
                data.tellCallback = tellCallback;
                data.seekCallback = seekCallback;

                setArtomatixSourceMGR(&jpeg_read_struct, data);
                jpeg_read_struct.err = jpeg_std_error(&err_mgr.pub);

                ArtomatixErrorStruct jerr;
                jpeg_read_struct.err = jpeg_std_error(&jerr.pub);
                //jpeg_read_struct.err->emit_message = JPEGCallbackFunctions::lessAnnoyingEmitMessage;
                jpeg_read_struct.err->error_exit = JPEGCallbackFunctions::handleFatalError;

                ArtomatixErrorStruct * err_ptr = (ArtomatixErrorStruct *) jpeg_read_struct.err;
                if (setjmp(err_ptr->buf))
                {
                    mErrorDetails = "[AImg::JPEGImageLoader::JPEGFile::openImage] jpeg_read_header failed!";

                    return AImgErrorCode::AIMG_LOAD_FAILED_EXTERNAL;
                }

                jpeg_read_struct.err->emit_message = JPEGCallbackFunctions::lessAnnoyingEmitMessage;
                jpeg_read_struct.err->error_exit = JPEGCallbackFunctions::handleFatalError;
                jpeg_read_header(&jpeg_read_struct, TRUE);

				return AImgErrorCode::AIMG_SUCCESS;
            }

            virtual int32_t getImageInfo(int32_t *width, int32_t *height, int32_t *numChannels, int32_t *bytesPerChannel, int32_t *floatOrInt, int32_t *decodedImgFormat)
            {
                *width = jpeg_read_struct.image_width;
                *height = jpeg_read_struct.image_height;
                *bytesPerChannel = 1;
                *numChannels = jpeg_read_struct.num_components;
                *floatOrInt = AImgFloatOrIntType::FITYPE_INT;
                *decodedImgFormat = ((int)AImgFormat::R8U) + jpeg_read_struct.num_components - 1;
                return AImgErrorCode::AIMG_SUCCESS;
            }

            virtual int32_t decodeImage(void *realDestBuffer, int32_t forceImageFormat)
            {
                void* destBuffer = realDestBuffer;

                std::vector<uint8_t> convertTmpBuffer(0);
                if(forceImageFormat != AImgFormat::INVALID_FORMAT && forceImageFormat != AImgFormat::RGB8U)
                {
                    int32_t numChannels, bytesPerChannel, floatOrInt;
                    AIGetFormatDetails(AImgFormat::RGB8U, &numChannels, &bytesPerChannel, &floatOrInt);

                    convertTmpBuffer.resize(jpeg_read_struct.image_width * jpeg_read_struct.image_height * bytesPerChannel * numChannels);
                    destBuffer = &convertTmpBuffer[0];
                }

                ArtomatixErrorStruct jerr;
                jpeg_read_struct.err = jpeg_std_error(&jerr.pub);
                jpeg_read_struct.err->emit_message = JPEGCallbackFunctions::lessAnnoyingEmitMessage;
                jpeg_read_struct.err->error_exit = JPEGCallbackFunctions::handleFatalError;

                 ArtomatixErrorStruct * err_ptr = (ArtomatixErrorStruct *) jpeg_read_struct.err;

                if (setjmp(err_ptr->buf))
                {
                    mErrorDetails = "[AImg::JPEGImageLoader::JPEGFile::decodeImage] jpeg_start_decompress failed!";
                    return AImgErrorCode::AIMG_LOAD_FAILED_EXTERNAL;
                }

                jpeg_start_decompress(&jpeg_read_struct);

                int row_stride = jpeg_read_struct.output_components * jpeg_read_struct.output_width;

                JSAMPROW buffer[1];

                buffer[0] = (JSAMPROW) destBuffer;

                if (setjmp(err_ptr->buf))
                {
                    mErrorDetails = "[AImg::JPEGImageLoader::JPEGFile::decodeImage] jpeg_read_scanlines failed!";
                    return AImgErrorCode::AIMG_LOAD_FAILED_EXTERNAL;
                }

                while (jpeg_read_struct.output_scanline < jpeg_read_struct.output_height)
                {
                    jpeg_read_scanlines(&jpeg_read_struct, buffer, 1);
                    buffer[0] = (uint8_t * )buffer[0] + row_stride;
                }

                jpeg_finish_decompress(&jpeg_read_struct);

                if (forceImageFormat != AImgFormat::INVALID_FORMAT && forceImageFormat != AImgFormat::RGB8U)
                {
                    int32_t err = AImgConvertFormat(destBuffer, realDestBuffer, jpeg_read_struct.image_width, jpeg_read_struct.image_height, AImgFormat::RGB8U, forceImageFormat);
                    if(err != AImgErrorCode::AIMG_SUCCESS)
                        return err;
                }

                return AImgErrorCode::AIMG_SUCCESS;
            }

            int32_t writeImage(void *data, int32_t width, int32_t height, int32_t inputFormat, WriteCallback writeCallback, TellCallback tellCallback, SeekCallback seekCallback, void *callbackData, void* encodingOptions)
            {
                AIL_UNUSED_PARAM(encodingOptions);

                std::vector<uint8_t> convertBuffer(0);
                if (inputFormat != AImgFormat::RGB8U)
                {
                    convertBuffer.resize(width * height * 3);

                    int32_t convertError = AImgConvertFormat(data, &convertBuffer[0], width, height, inputFormat, AImgFormat::RGB8U);

                    if (convertError != AImgErrorCode::AIMG_SUCCESS)
                        return convertError;
                    data = &convertBuffer[0];
                }

                CallbackData dataStruct;
                dataStruct.writeCallback = writeCallback;
                dataStruct.tellCallback = tellCallback;
                dataStruct.seekCallback = seekCallback;
                dataStruct.callbackData = callbackData;

                ArtomatixErrorStruct jerr;
                jpeg_compress_struct cinfo;
                cinfo.err = jpeg_std_error(&jerr.pub);
                cinfo.err->emit_message = JPEGCallbackFunctions::lessAnnoyingEmitMessage;
                cinfo.err->error_exit = JPEGCallbackFunctions::handleFatalError;
                jpeg_create_compress(&cinfo);

                setArtomatixDestinationMGR(&cinfo, dataStruct);

                cinfo.image_width = width;
                cinfo.image_height = height;
                cinfo.input_components = 3;
                cinfo.in_color_space = JCS_RGB;

                jpeg_set_defaults(&cinfo);

                jpeg_set_quality(&cinfo, JPEGConsts::Quality, TRUE);

                if (setjmp(jerr.buf))
                {
                    mErrorDetails = "[AImg::JPEGImageLoader::JPEGFile::writeImage] jpeg_start_compress failed!";
                    return AImgErrorCode::AIMG_LOAD_FAILED_EXTERNAL;
                }
                jpeg_start_compress(&cinfo, TRUE);

                int row_stride = width * cinfo.input_components;

                JSAMPROW row_pointer[1];


                if (setjmp(jerr.buf))
                {
                    mErrorDetails = "[AImg::JPEGImageLoader::JPEGFile::writeImage] jpeg_write_scanlines failed!";
                    return AImgErrorCode::AIMG_LOAD_FAILED_EXTERNAL;
                }

                while(cinfo.next_scanline < cinfo.image_height)
                {
                    row_pointer[0] = (uint8_t *)data + row_stride * cinfo.next_scanline;
                    jpeg_write_scanlines(&cinfo, row_pointer, 1);
                }

                jpeg_finish_compress(&cinfo);
                jpeg_destroy_compress(&cinfo);

                return AImgErrorCode::AIMG_SUCCESS;
            }
    };

    AImgFormat JPEGImageLoader::getWhatFormatWillBeWrittenForData(int32_t inputFormat)
    {
        AIL_UNUSED_PARAM(inputFormat);
        return AImgFormat::RGB8U;
    }

    AImgBase* JPEGImageLoader::getAImg()
    {
        return new JPEGFile();
    }
}

#endif // HAVE_JPEG
