cmake_minimum_required(VERSION 3.15)

# set(CMAKE_VERBOSE_MAKEFILE 1)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 14)

project(ccore)

string(TOLOWER "${CMAKE_BUILD_TYPE}" _cmake_build_type)
if("${CMAKE_GENERATOR}" STREQUAL "Ninja" AND "${_cmake_build_type}" STREQUAL "pydebug")
    include("cmake/pydebug.cmake")
endif()

### pyd file compile & link
find_package(Python3 COMPONENTS Interpreter Development)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${Python3_SITELIB}/skbuild/resources/cmake)
    find_package(PythonExtensions REQUIRED)

    add_library(_ccore MODULE ccore/_ccore.cxx)
        set_target_properties(_ccore PROPERTIES LINK_SEARCH_START_STATIC ON)
        set_target_properties(_ccore PROPERTIES LINK_SEARCH_END_STATIC ON)
        # target_include_directories(_ccore PRIVATE ${CMAKE_INSTALL_PREFIX}/include)
        # target_link_directories(_ccore PRIVATE ${Python3_LIBRARY_DIRS} ${CMAKE_INSTALL_PREFIX}/lib)

    python_extension_module(_ccore)

    ## save to -DCMAKE_INSTALL_PREFIX
    install(
        TARGETS _ccore
        LIBRARY DESTINATION ${CMAKE_PROJECT_NAME})

