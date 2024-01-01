cmake_minimum_required(VERSION 3.0.0)
project(rawdevpp VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)

include(CTest)
enable_testing()

find_package(Eigen3)

include_directories(${EIGEN3_INCLUDE_DIRS})

add_library(rawdevpp INTERFACE)
target_include_directories(rawdevpp INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# test
find_package(fmt)

file(GLOB_RECURSE test_src "test/*.cpp")
add_executable(rawdevpp-test ${test_src})

target_link_libraries(rawdevpp-test
    rawdevpp
    ${EIGEN3_INCLUDE_DIRS}
    fmt::fmt
)

# install
install(TARGETS rawdevpp
    EXPORT rawdevpp-targets
    PUBLIC_HEADER DESTINATION include
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/rawdevpp-config-version.cmake"
    VERSION ${CMAKE_PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/rawdevpp-config.cmake.in"
    "${PROJECT_BINARY_DIR}/rawdevpp-config.cmake"
    INSTALL_DESTINATION lib/cmake/rawdevpp
)

install(EXPORT rawdevpp-targets DESTINATION lib/cmake/rawdevpp)
install(FILES "${PROJECT_BINARY_DIR}/rawdevpp-config-version.cmake"
    "${PROJECT_BINARY_DIR}/rawdevpp-config.cmake"
    DESTINATION lib/cmake/rawdevpp)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ DESTINATION include)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
