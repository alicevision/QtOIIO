set(SOURCES_files_QtOIIO
    QtOIIOHandler.cpp
    QtOIIOHandler.hpp
    QtOIIOPlugin.cpp
    QtOIIOPlugin.hpp
    )
source_group("QtOIIO" FILES ${SOURCES_files_QtOIIO})

add_definitions(-DQT_PLUGIN)
add_definitions(-DQT_SHARED)
add_definitions(-DQT_NO_DEBUG)

add_library(QtOIIOPlugin
    SHARED
    ${SOURCES_files_QtOIIO}
    )

target_include_directories(QtOIIOPlugin
    PUBLIC
    ${OPENIMAGEIO_INCLUDE_DIRS}
    )
target_link_libraries(QtOIIOPlugin
    PUBLIC
    ${OPENIMAGEIO_LIBRARIES}
    Qt5::Core Qt5::Widgets
    )

install(TARGETS QtOIIOPlugin DESTINATION imageformats)
install(FILES QtOIIOPlugin.json DESTINATION imageformats)
