# QtOIIO - OIIO plugin for Qt

QtOIIO is a C++ plugin providing an [OpenImageIO](http://github.com/OpenImageIO/oiio) backend for image IO in Qt.
It has been developed to visualize RAW images from DSLRs in [Meshroom](https://github.com/alicevision/meshroom), as well as some intermediate files of the [AliceVision](https://github.com/alicevision/AliceVision) framework stored in EXR format (i.e: depthmaps).

Continuous integration:
* Windows: [![Build status](https://ci.appveyor.com/api/projects/status/te46xg9oan317bdy/branch/develop?svg=true)](https://ci.appveyor.com/project/AliceVision/qtoiio/branch/develop)

## License

The project is released under MPLv2, see [**COPYING.md**](COPYING.md).


## Get the project

Get the source code:
```bash
git clone --recursive git://github.com/alicevision/QtOIIO
cd QtOIIO
```
See [**INSTALL.md**](INSTALL.md) to build and install the project.

## Usage
When added to the `QT_PLUGIN_PATH`, all supported image files will be loaded through this plugin.

This plugin also provides a QML Qt3D Entity to load depthmaps files stored in EXR format:

```js
import DepthMapEntity 1.0

Scene3D {
  DepthMapEntity {
    source: "depthmap.exr"
  }
}

```  
