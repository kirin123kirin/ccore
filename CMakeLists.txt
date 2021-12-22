cmake_minimum_required(VERSION 3.15)
# print variables
macro(printvars)
    message(STATUS "\n\n*** dump start cmake variables ***\n\n")
    get_cmake_property(_variableNames VARIABLES)

    foreach(_variableName ${_variableNames})
            message(STATUS "${_variableName}=${${_variableName}}")
    endforeach()
    message(STATUS "\n*** dump end ***")
endmacro()

# set(CMAKE_VERBOSE_MAKEFILE 1)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 14)

project(ccore)

string(TOLOWER "${CMAKE_BUILD_TYPE}" _cmake_build_type)
if("${CMAKE_GENERATOR}" STREQUAL "Ninja" AND "${_cmake_build_type}" STREQUAL "pydebug")
    include("cmake/pydebug.cmake")
endif()

### nkf download & build
find_library(libnkf_static_LIBPATH libnkf_static HINTS ${CMAKE_INSTALL_PREFIX}/lib)
if (NOT libnkf_static_LIBPATH)
    include(FetchContent)
        FetchContent_Declare(
            libnkf_static
            GIT_REPOSITORY https://github.com/kirin123kirin/nkf.git
            GIT_TAG origin/master
        )
        FetchContent_GetProperties(libnkf_static)
        
        if(NOT libnkf_static_POPULATED)
            FetchContent_Populate(libnkf_static)
            add_subdirectory(${libnkf_static_SOURCE_DIR} ${libnkf_static_BINARY_DIR})
            include_directories(${libnkf_static_SOURCE_DIR})
        endif()

    install(TARGETS libnkf_static)

endif()

### pyd file compile & link
find_package(Python3 COMPONENTS Interpreter Development)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${Python3_SITELIB}/skbuild/resources/cmake)
    find_package(PythonExtensions REQUIRED)

    add_library(_ccore MODULE ccore/_ccore.cxx)
        target_include_directories(_ccore PRIVATE ${CMAKE_INSTALL_PREFIX}/include)
        target_link_directories(_ccore PRIVATE ${Python3_LIBRARY_DIRS} ${CMAKE_INSTALL_PREFIX}/lib)

        target_link_libraries(_ccore libnkf_static)

    python_extension_module(_ccore)

    ## save to -DCMAKE_INSTALL_PREFIX
    install(
        TARGETS _ccore
        LIBRARY DESTINATION ${CMAKE_PROJECT_NAME})

