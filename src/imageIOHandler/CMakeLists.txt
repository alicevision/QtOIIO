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

target_link_libraries(QtOIIOPlugin
    PUBLIC
    OpenImageIO::OpenImageIO
    )

target_link_libraries(QtOIIOPlugin
    PUBLIC
    Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui
    )

install(TARGETS QtOIIOPlugin DESTINATION imageformats)
