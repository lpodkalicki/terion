find_package(GTest REQUIRED)
include(GoogleTest)

file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp")
add_executable(libterion_tests ${TEST_SOURCES})
target_compile_options(libterion_tests PRIVATE -Wall -Wextra -pedantic -Werror)
target_link_libraries(libterion_tests PRIVATE terion GTest::gtest_main)
gtest_discover_tests(libterion_tests)
