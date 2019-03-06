using System;
using System.ComponentModel;

namespace Artomatix.ImageLoader
{
    public enum AImgFloatOrIntType
    {
        FITYPE_UNKNOWN = -1,
        FITYPE_FLOAT = 0,
        FITYPE_INT = 1
    }

    public enum AImgFileFormat
    {
        UNKNOWN_IMAGE_FORMAT = -1,
        EXR_IMAGE_FORMAT = 1,
        PNG_IMAGE_FORMAT = 2,
        JPEG_IMAGE_FORMAT = 3,
        TGA_IMAGE_FORMAT = 4,
        TIFF_IMAGE_FORMAT = 5,
        HDR_IMAGE_FORMAT = 6
    };

    // format is [channels][bits per channel][U/F]
    // U means unsigned normalised, so eg 8U maps integer vals 0-255 to float range 0-1, F means an normal float value

    [Flags]
    public enum AImgFormat
    {
        INVALID_FORMAT = -1,

        _8BITS = 1 << 0,
        _16BITS = 1 << 5,
        _32BITS = 1 << 6,

        R = 1 << 1,
        RG = 1 << 2,
        RGB = 1 << 3,
        RGBA = 1 << 4,

        FLOAT_FORMAT = 1 << 7,

        R8U = R | _8BITS,
        RG8U = RG | _8BITS,
        RGB8U = RGB | _8BITS,
        RGBA8U = RGBA | _8BITS,

        R16U = R | _16BITS,
        RG16U = RG | _16BITS,
        RGB16U = RGB | _16BITS,
        RGBA16U = RGBA | _16BITS,

        R16F = R | _16BITS | FLOAT_FORMAT,
        RG16F = RG | _16BITS | FLOAT_FORMAT,
        RGB16F = RGB | _16BITS | FLOAT_FORMAT,
        RGBA16F = RGBA | _16BITS | FLOAT_FORMAT,

        R32F = R | _32BITS | FLOAT_FORMAT,
        RG32F = RG | _32BITS | FLOAT_FORMAT,
        RGB32F = RGB | _32BITS | FLOAT_FORMAT,
        RGBA32F = RGBA | _32BITS | FLOAT_FORMAT
    };

    public static class AImgFormatExtension
    {
        public static int sizeInBytes(this AImgFormat format)
        {
            return format.bytesPerChannel() * format.numChannels();
        }

        public static int bytesPerChannel(this AImgFormat format)
        {
            switch (format)
            {
                case AImgFormat.R8U:
                    return 1;

                case AImgFormat.RG8U:
                    return 1;

                case AImgFormat.RGB8U:
                    return 1;

                case AImgFormat.RGBA8U:
                    return 1;

                case AImgFormat.R16U:
                    return 2;

                case AImgFormat.RG16U:
                    return 2;

                case AImgFormat.RGB16U:
                    return 2;

                case AImgFormat.RGBA16U:
                    return 2;

                case AImgFormat.R16F:
                    return 2;

                case AImgFormat.RG16F:
                    return 2;

                case AImgFormat.RGB16F:
                    return 2;

                case AImgFormat.RGBA16F:
                    return 2;

                case AImgFormat.R32F:
                    return 4;

                case AImgFormat.RG32F:
                    return 4;

                case AImgFormat.RGB32F:
                    return 4;

                case AImgFormat.RGBA32F:
                    return 4;
            }

            throw new InvalidEnumArgumentException();
        }

        public static int numChannels(this AImgFormat format)
        {
            switch (format)
            {
                case AImgFormat.R8U:
                    return 1;

                case AImgFormat.RG8U:
                    return 2;

                case AImgFormat.RGB8U:
                    return 3;

                case AImgFormat.RGBA8U:
                    return 4;

                case AImgFormat.R16U:
                    return 1;

                case AImgFormat.RG16U:
                    return 2;

                case AImgFormat.RGB16U:
                    return 3;

                case AImgFormat.RGBA16U:
                    return 4;

                case AImgFormat.R16F:
                    return 1;

                case AImgFormat.RG16F:
                    return 2;

                case AImgFormat.RGB16F:
                    return 3;

                case AImgFormat.RGBA16F:
                    return 4;

                case AImgFormat.R32F:
                    return 1;

                case AImgFormat.RG32F:
                    return 2;

                case AImgFormat.RGB32F:
                    return 3;

                case AImgFormat.RGBA32F:
                    return 4;
            }

            throw new InvalidEnumArgumentException();
        }

        public static AImgFloatOrIntType floatOrInt(this AImgFormat format)
        {
            switch (format)
            {
                case AImgFormat.R8U:
                    return AImgFloatOrIntType.FITYPE_INT;

                case AImgFormat.RG8U:
                    return AImgFloatOrIntType.FITYPE_INT;

                case AImgFormat.RGB8U:
                    return AImgFloatOrIntType.FITYPE_INT;

                case AImgFormat.RGBA8U:
                    return AImgFloatOrIntType.FITYPE_INT;

                case AImgFormat.R16U:
                    return AImgFloatOrIntType.FITYPE_INT;

                case AImgFormat.RG16U:
                    return AImgFloatOrIntType.FITYPE_INT;

                case AImgFormat.RGB16U:
                    return AImgFloatOrIntType.FITYPE_INT;

                case AImgFormat.RGBA16U:
                    return AImgFloatOrIntType.FITYPE_INT;

                case AImgFormat.R16F:
                    return AImgFloatOrIntType.FITYPE_FLOAT;

                case AImgFormat.RG16F:
                    return AImgFloatOrIntType.FITYPE_FLOAT;

                case AImgFormat.RGB16F:
                    return AImgFloatOrIntType.FITYPE_FLOAT;

                case AImgFormat.RGBA16F:
                    return AImgFloatOrIntType.FITYPE_FLOAT;

                case AImgFormat.R32F:
                    return AImgFloatOrIntType.FITYPE_FLOAT;

                case AImgFormat.RG32F:
                    return AImgFloatOrIntType.FITYPE_FLOAT;

                case AImgFormat.RGB32F:
                    return AImgFloatOrIntType.FITYPE_FLOAT;

                case AImgFormat.RGBA32F:
                    return AImgFloatOrIntType.FITYPE_FLOAT;
            }

            throw new InvalidEnumArgumentException();
        }
    }
}