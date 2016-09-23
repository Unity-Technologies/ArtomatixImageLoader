import unittest
import AImg
import enums
import os
import numpy as np

imagesDir = os.path.abspath(os.path.dirname(os.path.abspath(__file__)) + "/../../tests/images")

class TestAImg(unittest.TestCase):
    
    def test_read_attrs(self):
        img = AImg.AImg(imagesDir + "/exr/grad_32.exr")

        self.assertEqual(img.detectedFileFormat, enums.AImgFileFormats["EXR_IMAGE_FORMAT"])
        self.assertEqual(img.width, 64)
        self.assertEqual(img.height, 32)
        self.assertEqual(img.decodedImgFormat, enums.AImgFormats["RGB32F"])

    def test_read_exr(self):
        img = AImg.AImg(imagesDir + "/exr/grad_32.exr")

        decoded = img.decode()

        groundTruth = np.fromfile(imagesDir + "/exr/grad_32.bin", dtype=np.float32)

        for y in range(img.height):
            for x in range(img.width):
                self.assertEqual(groundTruth[x + y*img.width], decoded[y][x][0])


if __name__ == "__main__":
    unittest.main()
