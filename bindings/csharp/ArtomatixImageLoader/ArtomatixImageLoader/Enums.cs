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
        TIFF_IMAGE_FORMAT = 5
    };

    // format is [channels][bits per channel][U/F]
    // U means unsigned normalised, so eg 8U maps integer vals 0-255 to float range 0-1, F means an normal float value
    public enum AImgFormat
    {
        INVALID_FORMAT = -1,

        R8U = 0,
        RG8U = 1,
        RGB8U = 2,
        RGBA8U = 3,

        R16U = 4,
        RG16U = 5,
        RGB16U = 6,
        RGBA16U = 7,

        R16F = 8,
        RG16F = 9,
        RGB16F = 10,
        RGBA16F = 11,

        R32F = 12,
        RG32F = 13,
        RGB32F = 14,
        RGBA32F = 15
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