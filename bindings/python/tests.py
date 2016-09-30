import unittest
import AImg
import os
import io
import numpy as np

imagesDir = os.path.abspath(os.path.dirname(os.path.abspath(__file__)) + "/../../test_images")

class TestAImg(unittest.TestCase):
    
    def test_read_attrs(self):
        img = AImg.AImg(imagesDir + "/exr/grad_32.exr")

        self.assertEqual(img.detectedFileFormat, AImg.AImgFileFormats["EXR_IMAGE_FORMAT"])
        self.assertEqual(img.width, 64)
        self.assertEqual(img.height, 32)
        self.assertEqual(img.decodedImgFormat, AImg.AImgFormats["RGB32F"])

    def test_read_exr(self):
        img = AImg.AImg(imagesDir + "/exr/grad_32.exr")
        decoded = img.decode()

        groundTruth = np.fromfile(imagesDir + "/exr/grad_32.bin", dtype=np.float32)

        for y in range(img.height):
            for x in range(img.width):
                self.assertEqual(groundTruth[x + y*img.width], decoded[y][x][0])

    def test_write_exr(self):
        img = AImg.AImg(imagesDir + "/exr/grad_32.exr")
        decoded = img.decode()
        
        outFile = io.BytesIO()

        AImg.write(outFile, decoded, AImg.AImgFileFormats["EXR_IMAGE_FORMAT"])

        outFile.seek(0)
        img2 = AImg.AImg(outFile)

        decoded2 = img2.decode()

        for y in range(img2.height):
            for x in range(img2.width):
                for c in range(decoded.shape[2]):
                    self.assertEqual(decoded2[y][x][c], decoded[y][x][c])

    def test_open_bad_file(self):
        with self.assertRaises(AImg.AImgUnsupportedFiletypeException) as context:
            img = AImg.AImg(__file__)


if __name__ == "__main__":
    unittest.main()
