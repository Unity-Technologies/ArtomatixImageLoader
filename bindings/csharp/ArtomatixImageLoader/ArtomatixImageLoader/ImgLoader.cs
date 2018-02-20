using System.Runtime.InteropServices;
using System;
using System.IO;
using System.Text;
using System.Runtime.CompilerServices;
using Stugo.Interop;

namespace Artomatix.ImageLoader
{
    internal class ImgLoader
    {
        internal delegate int ReadCallback(IntPtr callbackData, IntPtr dest, int count);

        internal delegate void WriteCallback(IntPtr callbackData, IntPtr src, Int32 count);

        internal delegate int TellCallback(IntPtr callbackData);

        internal delegate void SeekCallback(IntPtr callbackData, int pos);

        // native code functions

        public static string AImgGetLastErrorDetails(IntPtr img)
        {
            IntPtr cstr = NativeFuncs.inst._AImgGetErrorDetails(img);
            return Marshal.PtrToStringAnsi(cstr);
        }

        public static ReadCallback getReadCallback(Stream s)
        {
            return (_, dest, count) =>
            {
                int bytesRead;
                unsafe
                {
                    byte* destPtr = (byte*)dest.ToPointer();
                    UnmanagedMemoryStream writeStream = new UnmanagedMemoryStream(destPtr, 0, count, FileAccess.Write);

                    byte[] readData = new byte[count];
                    bytesRead = s.Read(readData, 0, count);
                    writeStream.Write(readData, 0, count);

                    writeStream.Close();
                }

                return bytesRead;
            };
        }

        public static WriteCallback getWriteCallback(Stream s)
        {
            return (_, src, count) =>
            {
                unsafe
                {
                    byte* srcPtr = (byte*)src.ToPointer();
                    UnmanagedMemoryStream readStream = new UnmanagedMemoryStream(srcPtr, count);

                    byte[] data = new byte[count];
                    readStream.Read(data, 0, count);

                    s.Write(data, 0, count);

                    readStream.Close();
                }
            };
        }

        public static TellCallback getTellCallback(Stream s)
        {
            return (_) =>
            {
                return (int)s.Position;
            };
        }

        public static SeekCallback getSeekCallback(Stream s)
        {
            return (_, pos) =>
            {
                s.Position = pos;
            };
        }
    }
}