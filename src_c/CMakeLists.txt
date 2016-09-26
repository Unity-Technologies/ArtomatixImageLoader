CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT (AIL)

add_library(AIL SHARED
	exr.cpp
    exr.h
    png.h
    png.cpp
    AIL.cpp
    AIL.h
    AIL_internal.h
    ImageLoaderBase.h
)

set(AIL_COMPILE_FLAGS "")
if(UNIX)
    set(AIL_COMPILE_FLAGS "-Wall -pedantic -Wextra -g -std=c++11 -Wno-unknown-pragmas")
endif()

set(PYTHON_ENABLED ON CACHE BOOL "enable python binding")
set(EXR_ENABLED ON CACHE BOOL "enable loading EXR files")
set(PNG_ENABLED ON CACHE BOOL "enable loading PNG files")

if(PYTHON_ENABLED)
	add_library(AIL_py SHARED python.c)
	set_target_properties(AIL_py PROPERTIES OUTPUT_NAME "ail_py_native" PREFIX "")

	find_package(PythonLibs 2 REQUIRED)
	find_package(NumPy REQUIRED)

	target_link_libraries(AIL_py
		AIL
		${PYTHON_LIBRARIES})
		
	include_directories(${PYTHON_INCLUDE_DIR} ${PYTHON_NUMPY_INCLUDE_DIR})
endif()

set_target_properties(AIL PROPERTIES COMPILE_FLAGS "${AIL_COMPILE_FLAGS}")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

if(EXR_ENABLED)
	find_package(OpenEXR REQUIRED)
	target_link_libraries(AIL ${OPENEXR_LIBRARIES})
	add_definitions(-DHAVE_EXR -DOPENEXR_DLL)
endif()
if(PNG_ENABLED)
	find_package(PNG REQUIRED)
	target_link_libraries(AIL ${PNG_LIBRARIES})
	add_definitions(-DHAVE_PNG)
endif()

target_compile_definitions(AIL PRIVATE -DIS_AIL_COMPILE)

include_directories(
	${OPENEXR_INCLUDE_DIRS}
	${PNG_INCLUDE_DIRS}
)

install (TARGETS AIL
         ARCHIVE DESTINATION lib
         LIBRARY DESTINATION lib
         RUNTIME DESTINATION lib)

add_subdirectory(tests)
