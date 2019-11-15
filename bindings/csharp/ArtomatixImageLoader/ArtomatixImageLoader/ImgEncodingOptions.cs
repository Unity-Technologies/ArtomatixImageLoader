using System;
using System.Runtime.InteropServices;

namespace Artomatix.ImageLoader.ImgEncodingOptions
{
    public interface IFormatEncodeOptions { }

    [StructLayout(LayoutKind.Sequential)]
    public struct PngEncodingOptions : IFormatEncodeOptions
    {
        private Int32 _type;
        private Int32 _compressionLevel;
        private Filter _filter;

        public Int32 type { get { return _type; } }
        public Int32 compressionLevel { get { return _compressionLevel; } }
        public Filter filter { get { return _filter; } }

        [Flags]
        public enum Filter : int
        {
            PNG_NO_FILTERS = 0x00,
            PNG_FILTER_NONE = 0x08,
            PNG_FILTER_SUB = 0x10,
            PNG_FILTER_UP = 0x20,
            PNG_FILTER_AVG = 0x40,
            PNG_FILTER_PAETH = 0x80,
            PNG_ALL_FILTERS = (PNG_FILTER_NONE | PNG_FILTER_SUB | PNG_FILTER_UP | PNG_FILTER_AVG | PNG_FILTER_PAETH)
        }

        public PngEncodingOptions(Int32 compressionLevel, Filter filter)
        {
            _compressionLevel = compressionLevel;
            _filter = filter;
            _type = (Int32)AImgFileFormat.PNG_IMAGE_FORMAT;
        }
    }
}