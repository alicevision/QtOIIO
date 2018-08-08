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
    )
target_link_libraries(QtOIIOPlugin
    PUBLIC
    ${OPENIMAGEIO_LIBRARIES}
    )
if(Qt5Core_FOUND)
  target_link_libraries(QtOIIOPlugin
      PUBLIC
      Qt5::Core Qt5::Gui
      )
endif()

install(TARGETS QtOIIOPlugin DESTINATION imageformats)
