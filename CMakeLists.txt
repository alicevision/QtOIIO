message("CMake version: ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}")
cmake_minimum_required(VERSION 3.4)
project(QtOIIO LANGUAGES CXX C)

# C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)


# OpenImageIO
find_package(OpenImageIO REQUIRED)
if(OPENIMAGEIO_FOUND)
  message(STATUS "OpenImageIO found.")
else()
  message(SEND_ERROR "Failed to find OpenImageIO.")
endif()


# Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON) # needed for automoc
find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
if(Qt5Core_FOUND)
  message(STATUS "Qt5 found.")
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
add_subdirectory(src/depthMapEntity)

