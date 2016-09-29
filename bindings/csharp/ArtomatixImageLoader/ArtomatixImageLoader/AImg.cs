using System;
using System.IO;
using System.Runtime.InteropServices;

namespace ArtomatixImageLoader
{
    public class AImg : IDisposable
    {
        IntPtr nativeHandle = IntPtr.Zero;
        Stream stream;
        bool doDisposeStream;
        bool disposed = false;

        ImgLoader.ReadCallback readCallback;
        ImgLoader.TellCallback tellCallback;
        ImgLoader.SeekCallback seekCallback;

        Int32 _width, _height;
        Int32 _numChannels;
        Int32 _bytesPerChannel;

        public int width { get { return _width; } }
        public int height { get { return _height; } }
        public int numChannels { get { return _numChannels; } }
        public int bytesPerChannel { get { return _bytesPerChannel; } }
        public AImgFloatOrIntType floatOrInt { get; private set; }
        public AImgFileFormat detectedFileFormat { get; private set; }
        public AImgFormat decodedImgFormat { get; private set; }

        public AImg(AImgFileFormat fmt)
        {
            nativeHandle = ImgLoader.AImgGetAImg((Int32)fmt);
        }

        /// <summary>
        /// Opens an AImg from a stream.
        /// </summary>
        /// <param name="doDisposeStream">If set to <c>true</c> stream will be disposed whne the AImg is disposed.</param>
        public AImg(Stream stream, bool doDisposeStream = true)
        {
            if (!stream.CanRead || !stream.CanSeek)
                throw new Exception("unusable stream");

            this.stream = stream;
            this.doDisposeStream = doDisposeStream;

            readCallback = ImgLoader.getReadCallback(stream);
            tellCallback = ImgLoader.getTellCallback(stream);
            seekCallback = ImgLoader.getSeekCallback(stream);

            Int32 detectedImageFormatTmp = 0;
            Int32 errCode = ImgLoader.AImgOpen(readCallback, tellCallback, seekCallback, IntPtr.Zero, out nativeHandle, out detectedImageFormatTmp);
            AImgException.checkErrorCode(nativeHandle, errCode);


            detectedFileFormat = (AImgFileFormat)detectedImageFormatTmp;

            Int32 floatOrIntTmp = 0;
            Int32 decodedImgFormatTmp = 0;
            ImgLoader.AImgGetInfo(nativeHandle, out _width, out _height, out _numChannels, out _bytesPerChannel, out floatOrIntTmp, out decodedImgFormatTmp);
            floatOrInt = (AImgFloatOrIntType)floatOrIntTmp;
            decodedImgFormat = (AImgFormat)decodedImgFormatTmp;
        }

        /// <summary>
        /// Decodes the image into a user-specified buffer.
        /// </summary>
        /// <param name="destBuffer">This buffer can be of any struct type, so for example for an RGBA8 image, you might use a byte array,
        /// or a custom struct with byte fields for the r, g, b, and a components. A custom struct would need the [StructLayout(LayoutKind.Sequential), Serializable] attribute.</param>
        public void decodeImage<T>(T[] destBuffer, AImgFormat forceImageFormat = AImgFormat.INVALID_FORMAT) where T : struct
        {
            uint size = (uint)(Marshal.SizeOf(default(T)));

            if (size * destBuffer.Length < decodedImgFormat.sizeInBytes() * width * height)
                throw new ArgumentException("destBuffer is too small for this image");

            GCHandle pinnedArray = GCHandle.Alloc(destBuffer, GCHandleType.Pinned);
            IntPtr pointer = pinnedArray.AddrOfPinnedObject();

            Int32 errCode = ImgLoader.AImgDecodeImage(nativeHandle, pointer, (Int32)forceImageFormat);
            AImgException.checkErrorCode(nativeHandle, errCode);


            pinnedArray.Free();
        }

        public static AImgFormat getWhatFormatWillBeWrittenForData(AImgFileFormat fileFormat, AImgFormat format)
        {
            return (AImgFormat)ImgLoader.AImgGetWhatFormatWillBeWrittenForData((Int32)fileFormat, (Int32)format);
        }

        public void writeImage<T>(T[] data, int width, int height, AImgFormat format, Stream s) where T : struct
        {
            var writeCallback = ImgLoader.getWriteCallback(s);
            var tellCallback = ImgLoader.getTellCallback(s);
            var seekCallback = ImgLoader.getSeekCallback(s);

            GCHandle pinnedArray = GCHandle.Alloc(data, GCHandleType.Pinned);
            IntPtr pointer = pinnedArray.AddrOfPinnedObject();

            Int32 errCode = ImgLoader.AImgWriteImage(nativeHandle, pointer, width, height, (Int32)format, writeCallback, tellCallback, seekCallback, IntPtr.Zero);
            AImgException.checkErrorCode(nativeHandle, errCode);

            pinnedArray.Free();

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

                ImgLoader.AImgClose(nativeHandle);

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