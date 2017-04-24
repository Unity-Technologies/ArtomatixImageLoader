from distutils.core import setup, Extension
from distutils.command.build_ext import build_ext
import shutil
import os

thisFolder = os.path.dirname(os.path.realpath(__file__))

# The .so is actually compiled with cmake, but we have to put a sources arg in here anyway.
# my_build_ext below hijacks the build command to build with cmake and then just copy the 
# prebuilt file instead of invoking a compiler itself
ail_py_native = Extension('ail_py_native', sources=['nonexistant_file.c'])

class my_build_ext(build_ext):

    def build_extension(self, ext):
        folder = os.path.dirname(self.get_ext_fullpath(ext.name))
        
        if not os.path.exists(folder):
            os.makedirs(folder)

        so_path = self.build_native_with_cmake()
        shutil.copyfile(so_path, self.get_ext_fullpath(ext.name))


    def build_native_with_cmake(self):

        buildFolder = thisFolder + "/../../src_c/build_python"

        if not os.path.exists(buildFolder):
            os.makedirs(buildFolder)

        if os.system('cd "%s" && cmake ../ -DTESTS_ENABLED=OFF -DBUILD_SHARE_TYPE=STATIC -DPYTHON_ENABLED=On && make' % buildFolder) != 0:
            raise Exception("Building native code failed!")

        return buildFolder + '/ail_py_native.so'


setup(  name='AImg',
        version='0.22.0',
        package_dir={'': thisFolder},
        packages=['AImg'],
        cmdclass={'build_ext': my_build_ext },
        ext_modules=[ail_py_native],
)
