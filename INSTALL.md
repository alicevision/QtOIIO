# Development
This guide will help you build and install QtOIIO plugin.

## Requirements
QtOIIO requires:
* [Qt5](https://www.qt.io/) (>= 5.13, make sure to use the **same version** as the target application)
* [OpenImageIO](https://github.com/https://github.com/OpenImageIO/oiio) (>= 1.8.7) - with OpenEXR support for depthmaps visualization 
* [CMake](https://cmake.org/) (>= 3.4)
* On Windows platform: Microsoft Visual Studio (>= 2015.3)

> **Note for Windows**:
We recommend using [VCPKG](https://github.com/Microsoft/vcpkg) to get OpenImageIO. Qt version there is too old at the moment though, using official installer is necessary.

## Build instructions

In the following steps, replace <INSTALL_PATH> with the installation path of your choice.


#### Windows
> We will use "NMake Makefiles" generators here to have one-line build/installation,
but Visual Studio solutions can be generated if need be.

From a developer command-line, using Qt 5.13 (built with VS2015) and VCPKG for OIIO:
```
set QT_DIR=/path/to/qt/5.13.0/msvc2017_64
cmake .. -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake -DCMAKE_PREFIX_PATH=%QT_DIR% -DVCPKG_TARGET_TRIPLET=x64-windows -G "NMake Makefiles" -DCMAKE_INSTALL_PREFIX=<INSTALL_PATH> -DCMAKE_BUILD_TYPE=Release
nmake install
```

#### Linux

```bash
export QT_DIR=/path/to/qt/5.13.0/gcc_64
export OPENIMAGEIO_DIR=/path/to/oiio/install
export 
cmake .. -DCMAKE_PREFIX_PATH=$QT_DIR
-DOPENIMAGEIO_LIBRARY_DIR_HINTS:PATH=$OPENIMAGEIO_DIR/lib/ -DOPENIMAGEIO_INCLUDE_DIR:PATH=$OPENIMAGEIO_DIR/include/ -DCMAKE_INSTALL_PREFIX=<INSTALL_PATH> -DCMAKE_BUILD_TYPE=Release
make install
```

## Usage
Once built, setup those environment variables before launching your application:

```bash
# Windows:
set QML2_IMPORT_PATH=<INSTALL_PATH>/qml;%QML2_IMPORT_PATH%
set QT_PLUGIN_PATH=<INSTALL_PATH>;%QT_PLUGIN_PATH%

# Linux:
export QML2_IMPORT_PATH=<INSTALL_PATH>/qml:$QML2_IMPORT_PATH
export QT_PLUGIN_PATH=<INSTALL_PATH>:$QT_PLUGIN_PATH
```
