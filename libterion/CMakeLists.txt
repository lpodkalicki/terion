set(TARGET_NAME "terion")
file(GLOB_RECURSE TARGET_SOURCES "src/*.cpp")
add_library(${TARGET_NAME}_object OBJECT ${TARGET_SOURCES})
set_property(TARGET ${TARGET_NAME}_object PROPERTY POSITION_INDEPENDENT_CODE 1)
target_compile_options(${TARGET_NAME}_object PUBLIC -Wall -Wextra -pedantic -O3)
target_include_directories(${TARGET_NAME}_object
        PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        PUBLIC "$<INSTALL_INTERFACE:include>")

add_library(${TARGET_NAME} SHARED $<TARGET_OBJECTS:${TARGET_NAME}_object>)
target_link_libraries(${TARGET_NAME} "")
target_include_directories(${TARGET_NAME}
        PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        PUBLIC "$<INSTALL_INTERFACE:include>")

add_library(${TARGET_NAME}_static STATIC $<TARGET_OBJECTS:${TARGET_NAME}_object>)
target_link_libraries(${TARGET_NAME}_static "")
target_include_directories(${TARGET_NAME}_static
        PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        PUBLIC "$<INSTALL_INTERFACE:include>")

file(GLOB TARGET_PUBLIC_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/terion/*.h")
install(FILES ${TARGET_PUBLIC_HEADERS} DESTINATION include/terion COMPONENT development)

install(TARGETS ${TARGET_NAME} ${TARGET_NAME}_static
        EXPORT TerionConfig
        LIBRARY DESTINATION lib COMPONENT runtime
        ARCHIVE DESTINATION lib COMPONENT runtime
        RUNTIME DESTINATION bin COMPONENT runtime
        PUBLIC_HEADER DESTINATION include/terion COMPONENT development)

install(EXPORT TerionConfig FILE TerionConfig.cmake DESTINATION export NAMESPACE Terion:: COMPONENT development)

if (BUILD_TESTING)
    add_subdirectory(tests)
endif ()
