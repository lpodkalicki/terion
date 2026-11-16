set(TARGET_NAME "terion")
find_package(Threads REQUIRED)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

file(GLOB_RECURSE TARGET_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
add_library(${TARGET_NAME}_object OBJECT ${TARGET_SOURCES})
set_property(TARGET ${TARGET_NAME}_object PROPERTY POSITION_INDEPENDENT_CODE 1)
# Respect CMAKE_BUILD_TYPE: do not force Release optimization in Debug builds.
target_compile_options(${TARGET_NAME}_object PRIVATE -Wall -Wextra -pedantic)
target_include_directories(${TARGET_NAME}_object PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_library(${TARGET_NAME} SHARED $<TARGET_OBJECTS:${TARGET_NAME}_object>)
add_library(Terion::terion ALIAS ${TARGET_NAME})
target_link_libraries(${TARGET_NAME} PUBLIC Threads::Threads)
target_include_directories(${TARGET_NAME} PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

add_library(${TARGET_NAME}_static STATIC $<TARGET_OBJECTS:${TARGET_NAME}_object>)
add_library(Terion::terion_static ALIAS ${TARGET_NAME}_static)
target_link_libraries(${TARGET_NAME}_static PUBLIC Threads::Threads)
target_include_directories(${TARGET_NAME}_static PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

install(DIRECTORY include/terion/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/terion
    COMPONENT development FILES_MATCHING PATTERN "*.h")
install(TARGETS ${TARGET_NAME} ${TARGET_NAME}_static
    EXPORT TerionTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT runtime
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT development
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT runtime
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/TerionConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/TerionConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Terion")
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/TerionConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}" COMPATIBILITY SameMajorVersion)
install(EXPORT TerionTargets FILE TerionTargets.cmake
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Terion"
    NAMESPACE Terion:: COMPONENT development)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/TerionConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/TerionConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Terion"
    COMPONENT development)

if (BUILD_TESTING)
    add_subdirectory(tests)
endif ()
