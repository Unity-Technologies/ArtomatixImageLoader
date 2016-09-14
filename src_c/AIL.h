#ifndef ARTOMATIX_AIL_H
#define ARTOMATIX_AIL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" 
{
#endif

#ifdef WIN32
    #define CALLCONV __stdcall

	#ifdef IS_AIL_COMPILE
		#define EXPORT_FUNC  __declspec(dllexport)
	#else
		#define EXPORT_FUNC __declspec(dllimport)
	#endif
#else
    #define CALLCONV
	#define EXPORT_FUNC
#endif

///////////////////////
// Callback typedefs //
///////////////////////
typedef int32_t (CALLCONV *ReadCallback)    (void* callbackData, uint8_t* dest, int32_t count);
typedef void    (CALLCONV *WriteCallback)   (void* callbackData, const uint8_t* src, int32_t count);
typedef int32_t (CALLCONV *TellCallback)    (void* callbackData);
typedef void    (CALLCONV *SeekCallback)    (void* callbackData, int32_t pos);


////////////////
// Core enums //
////////////////

// format is [channels][bits per channel][U/F]
// U means unsigned normalised, so eg 8U maps integer vals 0-255 to float range 0-1, F means a normal float value
enum AImgFormat
{
    INVALID_FORMAT = -1,

    R8U     = 0,
    RG8U    = 1,
    RGB8U   = 2,
    RGBA8U  = 3,
    
    R16U    = 4,
    RG16U   = 5,
    RGB16U  = 6,
    RGBA16U = 7,

    R16F    = 8,
    RG16F   = 9,
    RGB16F  = 10,
    RGBA16F = 11,

    R32F    = 12,
    RG32F   = 13,
    RGB32F  = 14,
    RGBA32F = 15
};

enum AImgErrorCode
{
    AIMG_SUCCESS = 0,
    AIMG_UNSUPPORTED_FILETYPE = -1,
    AIMG_LOAD_FAILED_EXTERNAL = -2, // load failed in an external library
    AIMG_LOAD_FAILED_INTERNAL = -3, // load failed inside ArtomatixImageLoader
    AIMG_CONVERSION_FAILED_BAD_FORMAT = -4,
    AIMG_WRITE_FAILED_EXTERNAL = -5,
    AIMG_WRITE_FAILED_INTERNAL = -6,
    AIMG_LOAD_FAILED_UNSUPPORTED_TIFF = -7,
    AIMG_OPEN_FAILED_EMPTY_INPUT = -8,
    AIMG_INVALID_ENCODE_ARGS = -9
};

enum AImgFileFormat
{
    UNKNOWN_IMAGE_FORMAT = -1,
    EXR_IMAGE_FORMAT = 1,
    PNG_IMAGE_FORMAT = 2,
    JPEG_IMAGE_FORMAT = 3,
    TGA_IMAGE_FORMAT = 4,
    TIFF_IMAGE_FORMAT = 5
};

enum AImgFloatOrIntType
{
    FITYPE_UNKNOWN = -1,
    FITYPE_FLOAT = 0,
    FITYPE_INT = 1
};

/////////////////////////////
// Encoding option structs //
/////////////////////////////
//                         //
// - All option structs    //
// will have an int32 as   //
// their first member,     //
// which shall be required //
// to be set to the        //
// AImgFileFormat code for //
// that file format.       //
/////////////////////////////


// These defines copied from libpng's png.h
#define AIL_PNG_NO_FILTERS   0x00
#define AIL_PNG_FILTER_NONE  0x08
#define AIL_PNG_FILTER_SUB   0x10
#define AIL_PNG_FILTER_UP    0x20
#define AIL_PNG_FILTER_AVG   0x40
#define AIL_PNG_FILTER_PAETH 0x80
#define AIL_PNG_ALL_FILTERS  (AIL_PNG_FILTER_NONE | AIL_PNG_FILTER_SUB | AIL_PNG_FILTER_UP | AIL_PNG_FILTER_AVG | AIL_PNG_FILTER_PAETH)

struct PngEncodingOptions
{
    int32_t type;
    int32_t compressionLevel; // Used with png_set_compression_level()
    int32_t filter; // Used with png_set_filter(), set to some combination of AIL_PNG_ flag defines from above.
};

//////////////////////////
// Public API functions //
//////////////////////////

typedef void* AImgHandle;

EXPORT_FUNC const char* AImgGetErrorDetails(AImgHandle img);

// detectedFileFormat will be set to a member from AImgFileFormat if non-null, otherwise it is ignored.
EXPORT_FUNC int32_t AImgOpen(ReadCallback readCallback, TellCallback tellCallback, SeekCallback seekCallback, void* callbackData, AImgHandle* imgPtr, int32_t* detectedFileFormat);
EXPORT_FUNC void AImgClose(AImgHandle img);

EXPORT_FUNC int32_t AImgGetInfo(AImgHandle img, int32_t* width, int32_t* height, int32_t* numChannels, int32_t* bytesPerChannel, int32_t* floatOrInt, int32_t* decodedImgFormat, uint32_t *colourProfileLen);
EXPORT_FUNC int32_t AImgGetColourProfile(AImgHandle img, char* profileName, uint8_t* colourProfile, uint32_t *colourProfileLen);
EXPORT_FUNC int32_t AImgDecodeImage(AImgHandle img, void* destBuffer, int32_t forceImageFormat);
EXPORT_FUNC int32_t AImgInitialise();
EXPORT_FUNC void AImgCleanUp();

EXPORT_FUNC void AIGetFormatDetails(int32_t format, int32_t* numChannels, int32_t* bytesPerChannel, int32_t* floatOrInt);
EXPORT_FUNC int32_t AImgConvertFormat(void* src, void* dest, int32_t width, int32_t height, int32_t inFormat, int32_t outFormat);

EXPORT_FUNC int32_t AImgGetWhatFormatWillBeWrittenForData(int32_t fileFormat, int32_t inputFormat);

EXPORT_FUNC AImgHandle AImgGetAImg(int32_t fileFormat);

// encodingOptions should be one of the encoding option structs detailed in the section above. It shoudl be the struct that corresponds to the image format being written.
EXPORT_FUNC int32_t AImgWriteImage(AImgHandle imgH, void* data, int32_t width, int32_t height, int32_t inputFormat, const char *profileName, uint8_t *colourProfile, uint32_t colourProfileLen,
                                    WriteCallback writeCallback, TellCallback tellCallback, SeekCallback seekCallback, void* callbackData, void* encodingOptions);

EXPORT_FUNC void AIGetSimpleMemoryBufferCallbacks(ReadCallback* readCallback, WriteCallback* writeCallback, TellCallback* tellCallback, SeekCallback* seekCallback, void** callbackData, void* buffer, int32_t size);
EXPORT_FUNC void AIDestroySimpleMemoryBufferCallbacks(ReadCallback readCallback, WriteCallback writeCallback, TellCallback tellCallback, SeekCallback seekCallback, void* callbackData);


#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
    #include <vector>
    EXPORT_FUNC void AIGetResizableMemoryBufferCallbacks(ReadCallback* readCallback, WriteCallback* writeCallback, TellCallback* tellCallback, SeekCallback* seekCallback, void** callbackData, std::vector<uint8_t>* vec);
#endif

#endif //ARTOMATIX_AIL_H
