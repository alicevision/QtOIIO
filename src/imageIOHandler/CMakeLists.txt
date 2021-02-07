set(SOURCES_files_QtOIIO
    QtOIIOHandler.cpp
    QtOIIOHandler.hpp
    QtOIIOPlugin.cpp
    QtOIIOPlugin.hpp
    )
source_group("QtOIIO" FILES ${SOURCES_files_QtOIIO})

add_library(QtOIIOPlugin
    SHARED
    ${SOURCES_files_QtOIIO}
    )

target_include_directories(QtOIIOPlugin
    PUBLIC
    ${OPENIMAGEIO_INCLUDE_DIRS}
    ${OPENEXR_INCLUDE_DIR}
    ${ILMBASE_INCLUDE_DIR}
    )
target_link_libraries(QtOIIOPlugin
    PUBLIC
    ${OPENIMAGEIO_LIBRARIES}
    )

target_link_libraries(QtOIIOPlugin
    PUBLIC
    Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui
    )

install(TARGETS QtOIIOPlugin DESTINATION imageformats)
