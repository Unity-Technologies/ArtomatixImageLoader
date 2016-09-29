using System;
using NUnit.Framework;
using ArtomatixImageLoader;
using System.Reflection;
using System.IO;
using System.Runtime.CompilerServices;

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
                        img.decodeImage(data2);

                        for (int i = 0; i < data.Length; i++)
                            Assert.AreEqual(data[i], data2[i]);
                    }
                }
            }
        }
    }
}