# Target srcs
file(GLOB_RECURSE TARGET_SRCS *.cpp *.cxx *.cc *.C *.c *.h *.hpp)

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Gui REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Qml REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Quick REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS 3DCore REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS 3DRender REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS 3DExtras REQUIRED)

# Target properties
add_library(depthMapEntityQmlPlugin SHARED ${TARGET_SRCS})
target_include_directories(depthMapEntityQmlPlugin
    PUBLIC
    ${OPENIMAGEIO_INCLUDE_DIRS}
    ${OPENEXR_INCLUDE_DIR}
    ${ILMBASE_INCLUDE_DIR}
    )
target_link_libraries(depthMapEntityQmlPlugin
    PUBLIC
    ${OPENIMAGEIO_LIBRARIES}
    )

target_link_libraries(depthMapEntityQmlPlugin
      PUBLIC
      Qt${QT_VERSION_MAJOR}::Core
      Qt${QT_VERSION_MAJOR}::Gui
      Qt${QT_VERSION_MAJOR}::Qml
      Qt${QT_VERSION_MAJOR}::Quick
      Qt${QT_VERSION_MAJOR}::3DCore
      Qt${QT_VERSION_MAJOR}::3DRender
      Qt${QT_VERSION_MAJOR}::3DExtras
      )

# QT5_USE_MODULES(depthMapEntityQmlPlugin Core Qml Quick 3DCore 3DRender 3DExtras ${OPENIMAGEIO_LIBRARIES})


# Install settings
install(FILES "qmldir"
        DESTINATION ${CMAKE_INSTALL_PREFIX}/qml/DepthMapEntity)
install(TARGETS depthMapEntityQmlPlugin
        DESTINATION "${CMAKE_INSTALL_PREFIX}/qml/DepthMapEntity")
