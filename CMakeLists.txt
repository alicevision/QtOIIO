message("CMake version: ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}")
cmake_minimum_required(VERSION 3.4)
project(QtOIIO LANGUAGES CXX C)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
# C++11 for Qt5
if(${QT_VERSION_MAJOR} EQUAL 5)
    set(CMAKE_CXX_STANDARD 11)
endif()

# C++17 for Qt6
if(${QT_VERSION_MAJOR} EQUAL 6)
    set(CMAKE_CXX_STANDARD 17)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)


# OpenImageIO
find_package(OpenImageIO REQUIRED)
if(TARGET OpenImageIO::OpenImageIO)
    message(STATUS "OpenImageIO found.")
else()
    message(SEND_ERROR "Failed to find OpenImageIO.")
endif()


# Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON) # needed for automoc

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Gui REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)


if(Qt${QT_VERSION_MAJOR}Core_FOUND)
    message(STATUS "Qt ${QT_VERSION_MAJOR}.${QT_VERSION_MINOR} found.")
endif()

add_definitions(-DQT_PLUGIN)
add_definitions(-DQT_SHARED)
add_definitions(-DQT_NO_DEBUG)

# comment to get qDebug outputs
add_definitions(-DQT_NO_DEBUG_OUTPUT)

# comment to handle all possible file formats
# by default: jpeg, png and ico are handled by Qt (for performance reasons)
add_definitions(-DQTOIIO_USE_FORMATS_BLACKLIST)

add_subdirectory(src/imageIOHandler)

# TODO: Make it works for Qt6
# Add to Qt5 only for the moment since 3dcore
# is not part of the distribution anymore.
# Source: https://www.kdab.com/qt-3d-changes-in-qt-6/
if(Qt5_FOUND)
    add_subdirectory(src/depthMapEntity)
endif()
# add_subdirectory(src/depthMapEntity)
