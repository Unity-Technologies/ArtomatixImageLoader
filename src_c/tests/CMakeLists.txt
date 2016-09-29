set(AIMG_TESTS_ENABLED ON CACHE BOOL "enable tests")

if(AIMG_TESTS_ENABLED)
    # Setup gtest
    
    set(GTEST_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/gtest")
    set(LIBPREFIX "${CMAKE_STATIC_LIBRARY_PREFIX}")
    set(LIBSUFFIX "${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(GTEST_LOCATION "${GTEST_PREFIX}/src/gtest-build")
    set(GTEST_INCLUDES "${GTEST_PREFIX}/src/gtest/include")
    set(GTEST_LIBRARY  "${GTEST_LOCATION}/${LIBPREFIX}gtest${LIBSUFFIX}")
    set(GTEST_MAINLIB  "${GTEST_LOCATION}/${LIBPREFIX}gtest_main${LIBSUFFIX}")
    set(GTEST_CMAKE_ARGS "")
    
    if(WIN32)
        set(GTEST_LIBRARY  "optimized;${GTEST_LOCATION}/Release/${LIBPREFIX}gtest${LIBSUFFIX};debug;${GTEST_LOCATION}/Debug/${LIBPREFIX}gtest${LIBSUFFIX}")
        set(GTEST_MAINLIB  "optimized;${GTEST_LOCATION}/Release/${LIBPREFIX}gtest_main${LIBSUFFIX};debug;${GTEST_LOCATION}/Debug/${LIBPREFIX}gtest_main${LIBSUFFIX}")
        set(GTEST_CMAKE_ARGS "-Dgtest_force_shared_crt=ON")
    endif()
    
    include(ExternalProject)
    ExternalProject_Add(gtest
      URL https://github.com/google/googletest/archive/release-1.7.0.zip
      PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gtest
      CMAKE_ARGS "${GTEST_CMAKE_ARGS}"
      INSTALL_COMMAND ""
    )

    find_package(Threads)
    
    set(all_tests)

    function(ail_add_test test_name link_libs in_aitest)
        add_executable("test_${test_name}" "${test_name}.cpp" "testCommon.cpp" "testCommon.h")

        add_dependencies("test_${test_name}" gtest)
        target_include_directories("test_${test_name}" PRIVATE ${GTEST_INCLUDES})
        target_compile_definitions("test_${test_name}" PRIVATE TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
        target_link_libraries("test_${test_name}" ${GTEST_LIBRARY} ${GTEST_MAINLIB} ${link_libs} ${CMAKE_THREAD_LIBS_INIT})
       
        set_target_properties("test_${test_name}" PROPERTIES COMPILE_FLAGS "${AIL_COMPILE_FLAGS}")

        if(in_aitest)
            list(APPEND all_tests COMMAND "test_${test_name}")
            set(all_tests "${all_tests}" PARENT_SCOPE)
        endif()
    endfunction(ail_add_test)

    # actual tests go here
    ail_add_test(exr AIL Yes)
	
	if(PNG_ENABLED)
		ail_add_test(png "AIL;${PNG_LIBRARIES}" Yes)
	endif()

    
    add_custom_target(aitest ${all_tests})
    set_target_properties(aitest PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)
endif()
