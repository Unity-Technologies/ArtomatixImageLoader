using Stugo.Interop;
using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;

namespace Artomatix.ImageLoader
{
    internal class NativeFuncs
    {
        private static NativeFuncs initNative()
        {
            var dllPath = Path.GetFullPath($"{AppDomain.CurrentDomain.BaseDirectory}/AIMG.dll");
#if DEBUG
            var zipStream = System.Reflection.Assembly.GetExecutingAssembly().GetManifestResourceStream("Artomatix.ImageLoader.embedded_files.binaries.zip");
            NativeBinaryManager.NativeBinaryManager.ExtractNativeBinary(zipStream, dllPath);
#endif
            UnmanagedModuleCollection.Instance.LoadModule<NativeFuncs>(dllPath);
            var inst = UnmanagedModuleCollection.Instance.GetModule<NativeFuncs>();
            inst.AImgInitialise();

            return inst;
        }

        public static readonly NativeFuncs inst = initNative();
        // io callbacks

        // native code delegates
        public delegate Int32 AImgInitialise_t();

#pragma warning disable CS0649

        [EntryPoint("AImgInitialise")]
        public AImgInitialise_t AImgInitialise;

        public delegate IntPtr _AImgGetErrorDetails_t(IntPtr img);

        [EntryPoint("AImgGetErrorDetails")]
        public _AImgGetErrorDetails_t _AImgGetErrorDetails;

        public delegate IntPtr AImgGetAImg_t(Int32 fileFormat);

        [EntryPoint("AImgGetAImg")]
        public AImgGetAImg_t AImgGetAImg;

        internal delegate void AImgClose_t(IntPtr img);

        [EntryPoint("AImgClose")]
        public AImgClose_t AImgClose;

        public delegate Int32 AImgGetInfo_t(IntPtr img, out Int32 width, out Int32 height, out Int32 numChannels, out Int32 bytesPerChannel, out Int32 floatOrInt, out Int32 decodedImgFormat, out Int32 colourProfileLength);

        [EntryPoint("AImgGetInfo")]
        public AImgGetInfo_t AImgGetInfo;

        public delegate Int32 AImgGetColourProfile_t(IntPtr img, StringBuilder profileName, IntPtr colourProfile, out Int32 colourProfileLength);

        [EntryPoint("AImgGetColourProfile")]
        public AImgGetColourProfile_t AImgGetColourProfile;

        public delegate Int32 AImgDecodeImage_t(IntPtr img, IntPtr destBuffer, Int32 forceImageFormat);

        [EntryPoint("AImgDecodeImage")]
        public AImgDecodeImage_t AImgDecodeImage;

        public delegate void AImgCleanUp_t();

        [EntryPoint("AImgCleanUp")]
        public AImgCleanUp_t AImgCleanUp;

        public delegate Int32 AIChangeBitDepth_t(Int32 format, Int32 newBitDepth);

        [EntryPoint("AIChangeBitDepth")]
        public AIChangeBitDepth_t AIChangeBitDepth;

        public delegate Int32 AIGetBitDepth_t(Int32 format);

        [EntryPoint("AIGetBitDepth")]
        public AIGetBitDepth_t AIGetBitDepth;

        public delegate bool AImgIsFormatSupported_t(Int32 fileFormat, Int32 outputFormat);

        [EntryPoint("AImgIsFormatSupported")]
        public AImgIsFormatSupported_t AImgIsFormatSupported;

        public delegate Int32 AImgGetWhatFormatWillBeWrittenForData_t(Int32 fileFormat, Int32 inputFormat, Int32 outputFormat);

        [EntryPoint("AImgGetWhatFormatWillBeWrittenForData")]
        public AImgGetWhatFormatWillBeWrittenForData_t AImgGetWhatFormatWillBeWrittenForData;

        public delegate Int32 AImgOpen_t(
            [MarshalAs(UnmanagedType.FunctionPtr)] ImgLoader.ReadCallback readCallback,
            [MarshalAs(UnmanagedType.FunctionPtr)] ImgLoader.TellCallback tellCallback,
            [MarshalAs(UnmanagedType.FunctionPtr)] ImgLoader.SeekCallback seekCallback,
            IntPtr callbackData,
            out IntPtr img,
            out Int32 detectedFileFormat
        );

        [EntryPoint("AImgOpen")]
        public AImgOpen_t AImgOpen;

        public delegate Int32 AImgWriteImage_t(
            IntPtr img, IntPtr data, Int32 width, Int32 height, Int32 inputFormat, Int32 outputFormat,
            string profileName, IntPtr colourProfile, Int32 colourProfileLength,
            [MarshalAs(UnmanagedType.FunctionPtr)] ImgLoader.WriteCallback writeCallback,
            [MarshalAs(UnmanagedType.FunctionPtr)] ImgLoader.TellCallback tellCallback,
            [MarshalAs(UnmanagedType.FunctionPtr)] ImgLoader.SeekCallback seekCallback,
            IntPtr callbackData,
            IntPtr encodeOptions
        );

        [EntryPoint("AImgWriteImage")]
        public AImgWriteImage_t AImgWriteImage;

#pragma warning restore CS0649

        ~NativeFuncs()
        {
            NativeFuncs.inst.AImgCleanUp();
        }
    }
}