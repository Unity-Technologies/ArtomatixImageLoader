using System;
using NUnit.Framework;
using ArtomatixImageLoader;
using System.Reflection;
using System.IO;
using System.Runtime.CompilerServices;
using ArtomatixImageLoader.ImgEncodingOptions;

namespace ArtomatixImageLoaderTests
{
    [TestFixture]
    public class TestAImg
    {
        private static string getCsFilePath([CallerFilePath] string filePath = "")
        {
            return filePath;
        }

        private static string getImagesDir()
        {
            string thisFile = getCsFilePath();
            string thisDir = Path.GetDirectoryName(thisFile);

            return Path.GetFullPath(thisDir + "/../../../../test_images");
        }

        [Test]
        public static void TestReadAttrs()
        {
            using (AImg img = new AImg(File.Open(getImagesDir() + "/exr/grad_32.exr", FileMode.Open)))
            {
                Assert.AreEqual(64, img.width);
                Assert.AreEqual(32, img.height);
                Assert.AreEqual(4, img.bytesPerChannel);
                Assert.AreEqual(3, img.numChannels);
                Assert.AreEqual(AImgFloatOrIntType.FITYPE_FLOAT, img.floatOrInt);
                Assert.AreEqual(AImgFileFormat.EXR_IMAGE_FORMAT, img.detectedFileFormat);
                Assert.AreEqual(AImgFormat.RGB32F, img.decodedImgFormat);
            }
        }

        [Test]
        public static void TestReadImage()
        {
            using (AImg img = new AImg(File.Open(getImagesDir() + "/exr/grad_32.exr", FileMode.Open)))
            {
                float[] data = new float[img.width * img.height * img.decodedImgFormat.numChannels()];
                img.decodeImage(data);

                using (var dataStream = File.Open(getImagesDir() + "/exr/grad_32.bin", FileMode.Open))
                {
                    var reader = new BinaryReader(dataStream);
                    var realData = new float[dataStream.Length / 4];

                    for (int i = 0; i < realData.Length; i++)
                        realData[i] = reader.ReadSingle();

                    Assert.AreEqual(realData.Length * 3, data.Length);

                    for (int i = 0; i < realData.Length; i++)
                        Assert.AreEqual(realData[i], data[i*3]);
                }
            }
        }

        [Test]
        public static void TestForceImageFormat()
        {
            float[] fData;
            byte[] bData;

            using (AImg img = new AImg(File.Open(getImagesDir() + "/png/8-bit.png", FileMode.Open)))
            {
                fData = new float[img.width * img.height * 3];
                img.decodeImage(fData, AImgFormat.RGB32F);
            }

            using (AImg img = new AImg(File.Open(getImagesDir() + "/png/8-bit.png", FileMode.Open)))
            {
                bData = new byte[img.width * img.height * 3];
                img.decodeImage(bData, AImgFormat.RGB8U);

                for (int y = 0; y < img.height; y++)
                {
                    for(int x = 0; x < img.width; x++)
                    {
                        float groundTruth = ((float)bData[(x + y * img.width) * 3]) / 255.0f;
                        float forced = fData[(x + y * img.width) * 3];

                        Assert.AreEqual(groundTruth, forced);
                    }
                }
            }
        }

        [Test]
        public static void TestGetWhatFormatWIllBeWritten()
        {
            AImgFormat res = AImg.getWhatFormatWillBeWrittenForData(AImgFileFormat.EXR_IMAGE_FORMAT, AImgFormat.RGBA32F);
            Assert.AreEqual(AImgFormat.RGBA32F, res);
        }

        [Test]
        public static void TestWriteExr()
        {
            using (AImg img = new AImg(File.Open(getImagesDir() + "/exr/grad_32.exr", FileMode.Open)))
            {
                float[] data = new float[img.width * img.height * img.decodedImgFormat.numChannels()];
                img.decodeImage(data);

                using (var writeStream = new MemoryStream())
                {
                    var wImg = new AImg(AImgFileFormat.EXR_IMAGE_FORMAT);
                    wImg.writeImage(data, img.width, img.height, img.decodedImgFormat, writeStream);
                    writeStream.Seek(0, SeekOrigin.Begin);

                    using (AImg img2 = new AImg(writeStream))
                    {
                        float[] data2 = new float[img.width * img.height * img.decodedImgFormat.numChannels()];
                        img2.decodeImage(data2);

                        for (int i = 0; i < data.Length; i++)
                            Assert.AreEqual(data[i], data2[i]);
                    }
                }
            }
        }

        [Test]
        public static void TestWriteUncompressedPng()
        {
            using (AImg img = new AImg(File.Open(getImagesDir() + "/png/8-bit.png", FileMode.Open)))
            {
                byte[] data = new byte[img.width * img.height * img.decodedImgFormat.numChannels() * img.decodedImgFormat.bytesPerChannel()];
                img.decodeImage(data);

                using (var writeStream = new MemoryStream())
                {
                    var wImg = new AImg(AImgFileFormat.PNG_IMAGE_FORMAT);

                    PngEncodingOptions options = new PngEncodingOptions(0, PngEncodingOptions.Filter.PNG_NO_FILTERS);

                    wImg.writeImage(data, img.width, img.height, img.decodedImgFormat, writeStream, options);
                    writeStream.Seek(0, SeekOrigin.Begin);

                    using (AImg img2 = new AImg(writeStream))
                    {
                        byte[] data2 = new byte[img.width * img.height * img.decodedImgFormat.numChannels() * img.decodedImgFormat.bytesPerChannel()];
                        img2.decodeImage(data2);

                        for (int i = 0; i < data.Length; i++)
                            Assert.AreEqual(data[i], data2[i]);
                    }
                }
            }
        }

        [Test]
        public static void TestWrite32BitToPng()
        {
            using (AImg img = new AImg(File.Open(getImagesDir() + "/exr/grad_32.exr", FileMode.Open)))
            {
                float[] data = new float[img.width * img.height * img.decodedImgFormat.numChannels()];
                img.decodeImage(data);

                using (var writeStream = new MemoryStream())
                {
                    var wImg = new AImg(AImgFileFormat.PNG_IMAGE_FORMAT);
                    wImg.writeImage(data, img.width, img.height, img.decodedImgFormat, writeStream);
                    writeStream.Seek(0, SeekOrigin.Begin);

                    using (AImg img2 = new AImg(writeStream))
                    {
                        Assert.AreEqual(img2.decodedImgFormat, AImgFormat.RGBA16U);

                        UInt16[] data2 = new UInt16[img2.width * img2.height * img2.decodedImgFormat.numChannels()];
                        img2.decodeImage(data2);

                        for (int y = 0; y < img.height; y++)
                        {
                            for (int x = 0; x < img.width; x++)
                            {
                                var fR = data[((x + y * img.width) * img.decodedImgFormat.numChannels()) + 0];
                                var fG = data[((x + y * img.width) * img.decodedImgFormat.numChannels()) + 1];
                                var fB = data[((x + y * img.width) * img.decodedImgFormat.numChannels()) + 2];
                                var fA = 1.0f;

                                var hR = data2[((x + y * img.width) * img2.decodedImgFormat.numChannels()) + 0];
                                var hG = data2[((x + y * img.width) * img2.decodedImgFormat.numChannels()) + 1];
                                var hB = data2[((x + y * img.width) * img2.decodedImgFormat.numChannels()) + 2];
                                var hA = data2[((x + y * img.width) * img2.decodedImgFormat.numChannels()) + 3];

                                Assert.AreEqual((UInt16)(fR * 65535), hR);
                                Assert.AreEqual((UInt16)(fG * 65535), hG);
                                Assert.AreEqual((UInt16)(fB * 65535), hB);
                                Assert.AreEqual((UInt16)(fA * 65535), hA);
                            }
                        }
                    }
                }
            }
        }

        [Test]
        public static void TestOpenEmptyStream()
        {
            Assert.Throws<AImgOpenFailedEmptyInputException>(delegate
            {
                var s = new MemoryStream();
                AImg img = new AImg(s);
            });
        }
    }
}