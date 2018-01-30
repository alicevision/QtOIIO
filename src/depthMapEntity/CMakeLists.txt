# Target srcs
file(GLOB_RECURSE TARGET_SRCS *.cpp *.cxx *.cc *.C *.c *.h *.hpp)


find_package(Qt5Gui)
find_package(Qt5Qml)
find_package(Qt5Quick)
find_package(Qt53DCore)
find_package(Qt53DRender)
find_package(Qt53DExtras)


# Target properties
add_library(depthMapEntityQmlPlugin SHARED ${TARGET_SRCS})
target_include_directories(depthMapEntityQmlPlugin
    PUBLIC
    ${OPENIMAGEIO_INCLUDE_DIRS}
    )
target_link_libraries(depthMapEntityQmlPlugin
    PUBLIC
    ${OPENIMAGEIO_LIBRARIES}
    )
if(Qt5Core_FOUND)
  target_link_libraries(depthMapEntityQmlPlugin
      PUBLIC
      Qt5::Core Qt5::Gui Qt5::Qml Qt5::Quick Qt5::3DCore Qt5::3DRender Qt5::3DExtras
      )
endif()

# QT5_USE_MODULES(depthMapEntityQmlPlugin Core Qml Quick 3DCore 3DRender 3DExtras ${OPENIMAGEIO_LIBRARIES})


# Install settings
install(FILES "qmldir"
        DESTINATION ${CMAKE_INSTALL_PREFIX}/qml/DepthMapEntity)
install(TARGETS depthMapEntityQmlPlugin
        DESTINATION "${CMAKE_INSTALL_PREFIX}/qml/DepthMapEntity")
