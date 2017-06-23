# Artomatix Image Loader
Artomatix Image loader (or ail, or aimg), is a cross-language library for reading and writing various image file formats, including 8bit-per-pixel and, crucially, HDR formats as well, and is available in C#, python and C.

## Supported image formats
- png (all variants: 8 or 16-bit)
- jpeg (RGB8U only)
- tiff (Most common tiff files supported for read, see src_c/tiff.cpp for details, can write all pixel formats currently supported in AImg)
- tga (all 8-bit formats)
- exr (all variants: 16/32 bit U/F)

## Supported pixel formats
- R8U
- RG8U
- RGB8U,
- RGBA8U


- R16U
- RG16U
- RGB16U
- RGBA16U


- R16F
- RG16F
- RGB16F
- RGBA16F


- R32F
- RG32F
- RGB32F
- RGBA32F

Format of the above is \[channels\]\[bits per channel\]\[U/F\], where U means unsigned normalised int, and F means unnormalised float.
So, for example an RGB8U pixel as a c struct woiuld look like:
```c
struct RGB8U_px
{
    uint8_t r,g,b;
};
```

While an RG32F pixel would look like:
```c
struct RG32F_px
{
    float r, g;
};
```

16F formats are referring to half precision floats (yes, they exist but are not supported in many languages natively).

## How does it work?
At it's core AImg is a C library. While the public header file is C, much of the implementation is actually in C++.
It provides an interface that abstracts away the specifics of reading and writing particular image file formats, and internally uses the native library for each format to do the actual work. So, for example, we use libpng fro loading png files, and OpenEXR for loading exr files. This means that we can give reasonably reliable guarantees of compatability with whatever files are thrown at us. Built on top of this plain C interface are wrapper libraries for python and C#.
A potential downside of this approach, is that we now have a bunch of dependencies that need to be installed, which in a typical C/C++ dev environment is a huge pain, needing to install packages from your os package manager, or on windows to just get binaries from somewhere, compiled with the correct msvc version, correct runtime... etc etc. So, to make this easier, we just statically link in these native format-specific libraries, using a tool called [hunter](https://github.com/ruslo/hunter). So, the whole thing becomes very easy, and you simply get a single binary. We have integrated the build system for the native code into the ppython package, and are using another project of ours, [CSharpNativeCodeBuilder](https://github.com/Artomatix/CSharpNativeCodeBuilder) to create the C# package. And so, we largely avoid the nasty issues that could be caused by this.

## How do I get it?
- Python
    - Go to the releases tab, copy the url to one of the .tar.gz bundles, and pip install it. For example, `pip install https://github.com/Artomatix/ArtomatixImageLoader/archive/0.22.0.tar.gz`
- C#
    - Install our [nuget package](https://www.nuget.org/packages/ArtomatixImageLoaders/)
- C++
    - There is unfortunately no very lovely way of doing this right now. Eventually I'd like to create a [hunter package](https://github.com/ruslo/hunter)

## Examples

### Python
```python
import AImg

# Read an image
img = AImg.AImg("test.png") # can be passed a string filename, or a python file-like object
decoded = img.decode() # decoded is now a numpy 2d array with the data in the appropriate format
# decoded = img.decode(forceImageFormat=AImg.AImgFormats["RGB8U"]) # force the format to decode, internally converting the data if necessary
```
```python
# Write an image
AImg.write("out.png", data, AImg.AImgFileFormats["EXR_IMAGE_FORMAT"]) # where data is a numpy 2d array
```

### C#
```c#
// Read an image

float[] fData;

using (AImg img = new AImg(File.Open(getImagesDir() + "/png/8-bit.png", FileMode.Open)))
{
    fData = new float[img.width * img.height * 3];
    img.decodeImage(fData, AImgFormat.RGB32F);
}
```
```C#
// Write an image

using (var writeStream = new MemoryStream())
{
    var wImg = new AImg(AImgFileFormat.PNG_IMAGE_FORMAT);
    wImg.writeImage(data, width, height, AImgFormat.RGB8U, writeStream); // where data is a byte[] of length width * height * 3 (3 channel rgb data)
}
```

### C++
```C++
#include <AIL.h>

...


// Read an image

ReadCallback readCallback = NULL;
WriteCallback writeCallback = NULL;
TellCallback tellCallback = NULL;
SeekCallback seekCallback = NULL;
void* callbackData = NULL;

// presume data is an std::vector<uint8_t> containing the raw image file data
// AIGetSimpleMemoryBufferCallbacks is a default IO implementation that just loadss from a buffer in memory. You can define your own callbacks
// and pass them in if this is not enough.
AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &data[0], data.size());

AImgHandle img = NULL;
AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &img, NULL);


int32_t width;
int32_t height;

// fNumChannels, fBytesPerChannel and fFloatOrInt represent what is actually stored in the file
// This is not necessarily what will be decoded by AImg (eg, 24-bit floating point tiffs, that will be docoded as 32 bit floats).
// Use imgFormat with AIGetFormatDetails to determine what will actually be decodedas below.
int32_t fNumChannels;
int32_t fBytesPerChannel;
int32_t fFloatOrInt; // This is actually an enum AImgFloatOrIntType
int32_t imgFmt; // This is actually an enum AImgFormat
AImgGetInfo(img, &width, &height, &numChannels, &bytesPerChannel, &floatOrInt, &imgFmt);


int32_t realNumChannels;
int32_t realBytesPerChannel;
int32_t realFloatOrInt; // This is actually an enum AImgFloatOrIntType

// get the real details of what will be decoded
AIGetFormatDetails(int32_t format, &realNumChannels, &realBytesPerChannel, &realFloatOrInt);

// somewhere to store the decoded data, with teh correct size calculated for the format
std::vector<uint8_t> imgData(width*height*realNumChannels*realBytesPerChannel, 0);

// Here we do the actual decode
int32_t error = AImgDecodeImage(img, &imgData[0], AImgFormat::INVALID_FORMAT); // INVALID_FORMAT here says decode to the defult format, eg RGB8U for a jpg, RGBA8U for a n 8-bit png with transparancy. Can force the library to convert by passing in an explicit format here
if (error != AImgErrorCode::AIMG_SUCCESS)
    std::cout << AImgGetErrorDetails(img) << std::endl;
 ```
 ```c++
// Write an image

std::vector<uint8_t> outputData;
AIGetResizableMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &outputData);

AImgHandle wImg = AImgGetAImg(AImgFileFormat::PNG_IMAGE_FORMAT);
    err = AImgWriteImage(wImg, imgData, width, height, AImgFormat::RGBA8U, writeCallback, tellCallback, seekCallback, callbackData, NULL); // where imgData is a pointer to a buffer of RGBA8U data, of size width*height
if(err != AImgErrorCode::AIMG_SUCCESS)
     std::cout << AImgGetErrorDetails(wImg) << std::endl;
```
