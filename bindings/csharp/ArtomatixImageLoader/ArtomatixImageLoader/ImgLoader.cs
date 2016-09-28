using System.Runtime.InteropServices;
using System;
using System.IO;
using System.Runtime.CompilerServices;

namespace ArtomatixImageLoader
{
    public class ImgLoader
    {
        // This instance and the constructor and finalizer below are just a way of ensuring
        // that the AImgInitialise and AImgCleanUp functions get called when they need to be.
        // The static constructoris there because when we just had the instantiation inline with
        // the declaration, the c# compiler on windows would optimise it out, even though
        // its constructor had side effects. That's also why we have no optimise
        // flags on the static constructor. yaaaaaaaaaay
        private static ImgLoader forceStaticConstructorCall = null;

        [MethodImpl(MethodImplOptions.NoOptimization | MethodImplOptions.NoInlining)]
        static ImgLoader()
        {
            forceStaticConstructorCall = new ImgLoader();
        }

        private ImgLoader()
        {
            // make compiler warnings go away
            if (forceStaticConstructorCall == null)
                forceStaticConstructorCall = null;

            using (var f = File.OpenWrite("libAIL.so"))
            {
                var rStream = System.Reflection.Assembly.GetExecutingAssembly().GetManifestResourceStream("ArtomatixImageLoader.embedded_files.native_code");
                rStream.CopyTo(f);
            }

            AImgInitialise();
        }

        ~ImgLoader()
        {
            AImgCleanUp();
        }


        [DllImport("libAIL.so", EntryPoint="GetLastErrorDetails")]
        static extern IntPtr _GetLastErrorDetails();

        public static string GetLastErrorDetails()
        {
            IntPtr cstr = _GetLastErrorDetails();
            return Marshal.PtrToStringAnsi(cstr);
        }

        [DllImport("libAIL.so")]
        public static extern Int32 AImgOpen(
            [MarshalAs(UnmanagedType.FunctionPtr)] ReadCallback readCallback,
            [MarshalAs(UnmanagedType.FunctionPtr)] TellCallback tellCallback,
            [MarshalAs(UnmanagedType.FunctionPtr)] SeekCallback seekCallback,
            IntPtr callbackData,
            out IntPtr img,
            out Int32 detectedFileFormat
        );

        [DllImport("libAIL.so")]
        public static extern void AImgClose(IntPtr img);

        [DllImport("libAIL.so")]
        public static extern Int32 AImgGetInfo(IntPtr img, out Int32 width, out Int32 height, out Int32 numChannels, out Int32 bytesPerChannel, out Int32 floatOrInt, out Int32 decodedImgFormat);

        [DllImport("libAIL.so")]
        public static extern Int32 AImgDecodeImage(IntPtr img, IntPtr destBuffer, Int32 forceImageFormat);

        [DllImport("libAIL.so")]
        static extern Int32 AImgInitialise();

        [DllImport("libAIL.so")]
        static extern void AImgCleanUp();

        [DllImport("libAIL.so")]
        public static extern Int32 AImgGetWhatFormatWillBeWrittenForData(Int32 fileFormat, Int32 inputFormat);

        [DllImport("libAIL.so")]
        public static extern Int32 AImgWriteImage(Int32 fileFormat, IntPtr data, Int32 width, Int32 height, Int32 inputFormat, 
            [MarshalAs(UnmanagedType.FunctionPtr)] WriteCallback writeCallback,
            [MarshalAs(UnmanagedType.FunctionPtr)] TellCallback tellCallback,
            [MarshalAs(UnmanagedType.FunctionPtr)] SeekCallback seekCallback, 
            IntPtr callbackData);

        public delegate int ReadCallback(IntPtr callbackData, IntPtr dest, int count);
        public delegate void WriteCallback(IntPtr callbackData, IntPtr src, Int32 count);
        public delegate int TellCallback(IntPtr callbackData);
        public delegate void SeekCallback(IntPtr callbackData, int pos);

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