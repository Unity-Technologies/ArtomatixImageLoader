#ifndef AIL_TEST
#define AIL_TEST

#include <string>
#include <stdio.h>
#include <vector>
#include <iostream>
#include "../AIL.h"
inline std::string getImagesDir()
{
    std::string thisFile = __FILE__;

	char dirSep = '/';
	#ifdef WIN32
		dirSep = '\\';
	#endif
    size_t pos = thisFile.find_last_of(dirSep);
    size_t filenameLength = thisFile.length() - pos;

    std::string thisFolder = thisFile.substr(0, thisFile.size() - filenameLength);

    return thisFolder + "/../../test_images";
}

template <typename T>
std::vector<T> readFile(const std::string& path)
{
    FILE* f = fopen(path.c_str(), "rb");
    if (f == NULL)
        std::cout << "Could not open file: " << path << std::endl;

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    std::vector<T> retval(size/sizeof(T));
    fread(&retval[0], 1, size, f);

    fclose(f);

    return retval;
}

bool detectImage(const std::string& path, int32_t format);
bool validateImageHeaders(const std::string & path, int32_t expectedWidth, int32_t expectedHeight, int32_t expectedNumChannels, int32_t expectedBytesPerChannel, int32_t expectedFloatOrInt, int32_t expectedFormat);
bool compareForceImageFormat(const std::string& path);


#endif
