cmake_minimum_required(VERSION 3.15)

if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()

project(LabelPrint VERSION 1.2.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(LABELPRINT_BUILD_DEMO "Build the LabelPrint demo executable" ON)
option(LABELPRINT_BUILD_TESTS "Build LabelPrint tests" ON)
option(LABELPRINT_WIN7_COMPAT "Build with Windows 7 compatible WINVER/_WIN32_WINNT values" OFF)
option(LABELPRINT_STATIC_MSVC_RUNTIME "Link the static MSVC runtime (/MT or /MTd)" OFF)

if(MSVC AND LABELPRINT_STATIC_MSVC_RUNTIME)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# ============================================================================
# Library target — this is what other projects link against
# ============================================================================
add_library(labelprint STATIC
    zpl_label.cpp
    src/labelprint/printer_profile.cpp
    src/backends/zpl_backend.cpp
    src/backends/tspl_backend.cpp
    src/backends/tspl_bitmap_backend.cpp
    src/labelprint/medical_label_print.cpp
    src/templates/medical_label.cpp
    src/transports/file_transport.cpp
    src/transports/windows_raw_transport.cpp
    src/transports/tcp9100_transport.cpp
)
add_library(LabelPrint::labelprint ALIAS labelprint)

target_include_directories(labelprint PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_include_directories(labelprint PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if(MSVC)
    target_compile_options(labelprint PRIVATE /utf-8)
endif()

if(WIN32)
    target_compile_definitions(labelprint PRIVATE NOMINMAX)
    target_link_libraries(labelprint PUBLIC gdiplus ws2_32 winspool)
endif()

if(WIN32 AND LABELPRINT_WIN7_COMPAT)
    target_compile_definitions(labelprint PUBLIC
        WINVER=0x0601
        _WIN32_WINNT=0x0601
    )
endif()

# ============================================================================
# Executable — sample / demo binary (not needed by consumers)
# ============================================================================
if(LABELPRINT_BUILD_DEMO)
    add_executable(labelprint_demo main.cpp)
    target_link_libraries(labelprint_demo PRIVATE labelprint)
    if(MSVC)
        target_compile_options(labelprint_demo PRIVATE /utf-8)
    endif()
endif()

# ============================================================================
# Tests
# ============================================================================
if(LABELPRINT_BUILD_TESTS)
    add_executable(test_runner
        test/test_main.cpp
        test/test_document.cpp
        test/test_backends.cpp
    )
    target_link_libraries(test_runner PRIVATE labelprint)
    if(MSVC)
        target_compile_options(test_runner PRIVATE /utf-8)
    endif()
endif()

# ============================================================================
# Install / release package
# ============================================================================
install(TARGETS labelprint
    EXPORT LabelPrintTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES README.md API.md DESTINATION .)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/LabelPrintConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/LabelPrintConfig.cmake
    INSTALL_DESTINATION cmake
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/LabelPrintConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/LabelPrintConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/LabelPrintConfigVersion.cmake
    DESTINATION cmake
)

install(EXPORT LabelPrintTargets
    FILE LabelPrintTargets.cmake
    NAMESPACE LabelPrint::
    DESTINATION cmake
)
