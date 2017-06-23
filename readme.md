# Artomatix Image Loader
Artomatix Image loader (or ail, or aimg), is a cross-language library for reading and writing various image file formats, including 8bit-per-pixel and, crucially, HDR formats ass well, and is available in C#, python and C.

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
