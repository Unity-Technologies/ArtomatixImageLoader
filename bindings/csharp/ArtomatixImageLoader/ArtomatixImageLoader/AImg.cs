using Artomatix.ImageLoader.ImgEncodingOptions;
using System;
using System.IO;
using System.Runtime.InteropServices;

namespace Artomatix.ImageLoader
{
    public class AImg : IDisposable
    {
        private IntPtr nativeHandle = IntPtr.Zero;
        private Stream stream;
        private bool doDisposeStream;
        private bool disposed = false;

        private ImgLoader.ReadCallback readCallback;
        private ImgLoader.TellCallback tellCallback;
        private ImgLoader.SeekCallback seekCallback;

        private Int32 _width, _height;
        private Int32 _numChannels;
        private Int32 _bytesPerChannel;

        public int width { get { return _width; } }
        public int height { get { return _height; } }
        public int numChannels { get { return _numChannels; } }
        public int bytesPerChannel { get { return _bytesPerChannel; } }
        public AImgFloatOrIntType floatOrInt { get; private set; }
        public AImgFileFormat detectedFileFormat { get; private set; }
        public AImgFormat decodedImgFormat { get; private set; }
        public string colourProfileName { get; private set; }
        public byte[] colourProfile { get; private set; }

        public AImg(AImgFileFormat fmt)
        {
            nativeHandle = NativeFuncs.inst.AImgGetAImg((Int32)fmt);
        }

        /// <summary>
        /// Opens an AImg from a stream.
        /// </summary>
        /// <param name="stream">Stream to create the <see cref="AImg"/> from</param>
        /// <param name="doDisposeStream">If set to <c>true</c> stream will be disposed whne the AImg is disposed.</param>
        public unsafe AImg(Stream stream, bool doDisposeStream = true)
        {
            if (!stream.CanRead || !stream.CanSeek)
                throw new Exception("unusable stream");

            this.stream = stream;
            this.doDisposeStream = doDisposeStream;

            readCallback = ImgLoader.GetReadCallback(stream);
            tellCallback = ImgLoader.GetTellCallback(stream);
            seekCallback = ImgLoader.GetSeekCallback(stream);

            Int32 detectedImageFormatTmp = 0;
            Int32 errCode = NativeFuncs.inst.AImgOpen(readCallback, tellCallback, seekCallback, IntPtr.Zero, out nativeHandle, out detectedImageFormatTmp);
            AImgException.checkErrorCode(nativeHandle, errCode);

            detectedFileFormat = (AImgFileFormat)detectedImageFormatTmp;

            Int32 floatOrIntTmp = 0;
            Int32 decodedImgFormatTmp = 0;
            Int32 colourProfileLen = 0;
            NativeFuncs.inst.AImgGetInfo(nativeHandle, out _width, out _height, out _numChannels, out _bytesPerChannel, out floatOrIntTmp, out decodedImgFormatTmp, out colourProfileLen);
            floatOrInt = (AImgFloatOrIntType)floatOrIntTmp;
            decodedImgFormat = (AImgFormat)decodedImgFormatTmp;
            colourProfile = new byte[colourProfileLen];
            fixed (byte* array = colourProfile)
            {
                System.Text.StringBuilder _colourProfileName = new System.Text.StringBuilder(30);
                NativeFuncs.inst.AImgGetColourProfile(nativeHandle, _colourProfileName, (IntPtr)array, out colourProfileLen);
                colourProfileName = _colourProfileName.ToString();
            }
        }

        /// <summary>
        /// Decodes the image into a user-specified buffer.
        /// </summary>
        /// <param name="destBuffer">This buffer can be of any
        /// struct type, so for example for an RGBA8 image, you might use a byte array,
        /// or a custom struct with byte fields for the r, g, b, and a components.
        /// A custom struct would need the <see cref="StructLayoutAttribute"/> with <see cref="LayoutKind.Sequential"/> </param>
        /// <param name="forceImageFormat">The format that is expected to be read into <paramref name="destBuffer"/></param>
        ///
        public void decodeImage<T>(T[] destBuffer, AImgFormat forceImageFormat = AImgFormat.INVALID_FORMAT) where T : struct
        {
            uint size = (uint)(Marshal.SizeOf(default(T)));

            if (size * destBuffer.Length < decodedImgFormat.sizeInBytes() * width * height)
                throw new ArgumentException("destBuffer is too small for this image");

            GCHandle pinnedArray = GCHandle.Alloc(destBuffer, GCHandleType.Pinned);
            IntPtr pointer = pinnedArray.AddrOfPinnedObject();

            Int32 errCode = NativeFuncs.inst.AImgDecodeImage(nativeHandle, pointer, (Int32)forceImageFormat);
            AImgException.checkErrorCode(nativeHandle, errCode);

            pinnedArray.Free();
        }

        public static bool IsFormatSupported(AImgFileFormat fileFormat, AImgFormat outputFormat)
        {
            return NativeFuncs.inst.AImgIsFormatSupported((Int32)fileFormat, (Int32)outputFormat);
        }

        public static AImgFormat getWhatFormatWillBeWrittenForData(AImgFileFormat fileFormat, AImgFormat inputFormat, AImgFormat outputFormat)
        {
            return (AImgFormat)NativeFuncs.inst.AImgGetWhatFormatWillBeWrittenForData((Int32)fileFormat, (Int32)inputFormat, (Int32)outputFormat);
        }

        public static AImgFormat ChangeFormatBitDepth(AImgFormat format, AImgFormat newBitDepth)
        {
            return (AImgFormat)NativeFuncs.inst.AIChangeBitDepth((Int32)format, (Int32)newBitDepth);
        }

        public static AImgFormat GetBitDepth(AImgFormat format)
        {
            return (AImgFormat)NativeFuncs.inst.AIGetBitDepth((Int32)format);
        }

        public unsafe void writeImage<T>(T[] data,
            int width,
            int height,
            AImgFormat inputFormat,
            string profileName, byte[] colourProfile,
            Stream s,
            IFormatEncodeOptions options = null) where T : struct
        {
            WriteImage(data, width, height,
                inputFormat, inputFormat,
               profileName, colourProfile,
               s, options);
        }

        public unsafe void WriteImage<T>(T[] data,
            int width,
            int height,
            AImgFormat inputFormat,
            AImgFormat outputFormat,
            string profileName, byte[] colourProfile,
            Stream stream,
            IFormatEncodeOptions options = null) where T : struct
        {
            var writeCallback = ImgLoader.GetWriteCallback(stream);
            var tellCallback = ImgLoader.GetTellCallback(stream);
            var seekCallback = ImgLoader.GetSeekCallback(stream);

            if (profileName == string.Empty)
            {
                profileName = "empty";
            }

            var pinnedArray = GCHandle.Alloc(data, GCHandleType.Pinned);
            var pointer = pinnedArray.AddrOfPinnedObject();

            fixed (byte* colourProfileArray = colourProfile)
            {
                var encodeOptionsHandle = default(GCHandle);
                var encodeOptionsPtr = IntPtr.Zero;

                if (options != null)
                {
                    encodeOptionsHandle = GCHandle.Alloc(options, GCHandleType.Pinned);
                    encodeOptionsPtr = encodeOptionsHandle.AddrOfPinnedObject();
                }
                try
                {
                    int colourProfileLength = 0;
                    if (colourProfile != null)
                    {
                        colourProfileLength = colourProfile.Length;
                    }

                    var errCode = NativeFuncs.inst.AImgWriteImage(nativeHandle, pointer, width, height,
                        (Int32)inputFormat, (Int32)outputFormat,
                        profileName, (IntPtr)colourProfileArray, colourProfileLength,
                        writeCallback, tellCallback, seekCallback, IntPtr.Zero, encodeOptionsPtr);
                    AImgException.checkErrorCode(nativeHandle, errCode);
                }
                finally
                {
                    pinnedArray.Free();

                    if (encodeOptionsPtr != IntPtr.Zero)
                        encodeOptionsHandle.Free();
                }
            }
            GC.KeepAlive(writeCallback);
            GC.KeepAlive(tellCallback);
            GC.KeepAlive(seekCallback);
        }

        private void Dispose(bool disposing)
        {
            if (!disposed)
            {
                disposed = true;

                if (disposing)
                    GC.SuppressFinalize(this);

                NativeFuncs.inst.AImgClose(nativeHandle);

                if (doDisposeStream)
                    stream.Dispose();
            }
        }

        public void Dispose()
        {
            Dispose(true);
        }

        ~AImg()
        {
            Dispose(false);
        }
    }
}