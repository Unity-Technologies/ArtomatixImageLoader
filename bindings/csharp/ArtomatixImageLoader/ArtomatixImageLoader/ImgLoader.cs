using System.Runtime.InteropServices;
using System;
using System.IO;
using System.Text;
using System.Runtime.CompilerServices;
using Stugo.Interop;

namespace ArtomatixImageLoader
{
    // io callbacks
    internal delegate int ReadCallback(IntPtr callbackData, IntPtr dest, int count);
    internal delegate void WriteCallback(IntPtr callbackData, IntPtr src, Int32 count);
    internal delegate int TellCallback(IntPtr callbackData);
    internal delegate void SeekCallback(IntPtr callbackData, int pos);

    // native code delegates
    internal delegate Int32 AImgInitialise();
    internal delegate IntPtr _AImgGetErrorDetails(IntPtr img);
    internal delegate IntPtr AImgGetAImg(Int32 fileFormat);
    internal delegate void AImgClose(IntPtr img);
    internal delegate Int32 AImgGetInfo(IntPtr img, out Int32 width, out Int32 height, out Int32 numChannels, out Int32 bytesPerChannel, out Int32 floatOrInt, out Int32 decodedImgFormat, out Int32 colorProfileLength);
    internal delegate Int32 AImgGetColourProfile(IntPtr img, StringBuilder profileName, IntPtr colorProfile, out Int32 colorProfileLength);
    internal delegate Int32 AImgDecodeImage(IntPtr img, IntPtr destBuffer, Int32 forceImageFormat);
    internal delegate void AImgCleanUp();
    internal delegate Int32 AImgGetWhatFormatWillBeWrittenForData(Int32 fileFormat, Int32 inputFormat);

    internal delegate Int32 AImgOpen(
        [MarshalAs(UnmanagedType.FunctionPtr)] ReadCallback readCallback,
        [MarshalAs(UnmanagedType.FunctionPtr)] TellCallback tellCallback,
        [MarshalAs(UnmanagedType.FunctionPtr)] SeekCallback seekCallback,
        IntPtr callbackData,
        out IntPtr img,
        out Int32 detectedFileFormat
    );
    internal delegate Int32 AImgWriteImage(
        IntPtr img, IntPtr data, Int32 width, Int32 height, Int32 inputFormat, string profileName, IntPtr colorProfile, Int32 colorProfileLength,
        [MarshalAs(UnmanagedType.FunctionPtr)] WriteCallback writeCallback,
        [MarshalAs(UnmanagedType.FunctionPtr)] TellCallback tellCallback,
        [MarshalAs(UnmanagedType.FunctionPtr)] SeekCallback seekCallback, 
        IntPtr callbackData,
        IntPtr encodeOptions
    );

    internal class ImgLoader
    {
        // This instance and the constructors and finalizer below are just a way of ensuring
        // that the AImgInitialise and AImgCleanUp functions get called when they need to be.
        // The static constructor is there because when we just had the instantiation inline with
        // the declaration, the c# compiler on windows would optimise it out, even though
        // its constructor had side effects. That's also why we have no optimise
        // flags on the static constructor. yaaaaaaaaaay
        private static ImgLoader forceStaticConstructorCall = null;

        [MethodImpl(MethodImplOptions.NoOptimization | MethodImplOptions.NoInlining)]
        static ImgLoader()
        {
            forceStaticConstructorCall = new ImgLoader();
        }

        // extracts the correct native code binary and loads the functions from it
        private ImgLoader()
        {
            // make compiler warnings go away
            if (forceStaticConstructorCall == null)
                forceStaticConstructorCall = null;

            string dllPath = Path.GetFullPath("libAIL.so");
            var asm = System.Reflection.Assembly.GetAssembly(typeof(ImgLoader));
            var zipStream = asm.GetManifestResourceStream("ArtomatixImageLoader.embedded_files.binaries.zip");

            NativeBinaryManager.NativeBinaryManager.ExtractNativeBinary(zipStream, dllPath);
            var loader = UnmanagedModuleLoaderBase.GetLoader(dllPath);

            AImgInitialise = (AImgInitialise)loader.GetDelegate("AImgInitialise", typeof(AImgInitialise));
            _AImgGetErrorDetails = (_AImgGetErrorDetails)loader.GetDelegate("AImgGetErrorDetails", typeof(_AImgGetErrorDetails));
            AImgGetAImg = (AImgGetAImg)loader.GetDelegate("AImgGetAImg", typeof(AImgGetAImg));
            AImgOpen = (AImgOpen)loader.GetDelegate("AImgOpen", typeof(AImgOpen));
            AImgClose = (AImgClose)loader.GetDelegate("AImgClose", typeof(AImgClose));
            AImgGetInfo = (AImgGetInfo)loader.GetDelegate("AImgGetInfo", typeof(AImgGetInfo));
            AImgGetColourProfile = (AImgGetColourProfile)loader.GetDelegate("AImgGetColourProfile", typeof(AImgGetColourProfile));
            AImgDecodeImage = (AImgDecodeImage)loader.GetDelegate("AImgDecodeImage", typeof(AImgDecodeImage));
            AImgCleanUp = (AImgCleanUp)loader.GetDelegate("AImgCleanUp", typeof(AImgCleanUp));
            AImgGetWhatFormatWillBeWrittenForData = (AImgGetWhatFormatWillBeWrittenForData)loader.GetDelegate("AImgGetWhatFormatWillBeWrittenForData", typeof(AImgGetWhatFormatWillBeWrittenForData));
            AImgWriteImage = (AImgWriteImage)loader.GetDelegate("AImgWriteImage", typeof(AImgWriteImage));

            AImgInitialise();
        }

        ~ImgLoader()
        {
            AImgCleanUp();

            try
            {
                File.Delete(Path.GetFullPath("libAIL.so"));
            }
            catch { }

        }

        // native code functions
        static _AImgGetErrorDetails _AImgGetErrorDetails = null;
        public static AImgGetAImg AImgGetAImg = null;
        public static AImgOpen AImgOpen = null;
        public static AImgClose AImgClose = null;
        public static AImgGetInfo AImgGetInfo = null;
        public static AImgGetColourProfile AImgGetColourProfile = null;
        public static AImgDecodeImage AImgDecodeImage = null;
        static AImgInitialise AImgInitialise = null;
        static AImgCleanUp AImgCleanUp = null;
        public static AImgGetWhatFormatWillBeWrittenForData AImgGetWhatFormatWillBeWrittenForData = null;
        public static AImgWriteImage AImgWriteImage = null;

        public static string AImgGetLastErrorDetails(IntPtr img)
        {
            IntPtr cstr = _AImgGetErrorDetails(img);
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
