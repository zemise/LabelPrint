cmake_minimum_required(VERSION 3.14)
project(LabelPrint VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# Library target — this is what other projects link against
# ============================================================================
add_library(labelprint STATIC
    zpl_label.cpp
    src/labelprint/printer_profile.cpp
    src/backends/zpl_backend.cpp
    src/backends/tspl_backend.cpp
    src/backends/tspl_bitmap_backend.cpp
    src/templates/medical_label.cpp
    src/transports/file_transport.cpp
    src/transports/windows_raw_transport.cpp
    src/transports/tcp9100_transport.cpp
)

target_include_directories(labelprint PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if(MSVC)
    target_compile_options(labelprint PRIVATE /utf-8)
    target_link_libraries(labelprint PUBLIC gdiplus.lib)
endif()

# ============================================================================
# Executable — sample / demo binary (not needed by consumers)
# ============================================================================
add_executable(labelprint_demo main.cpp)
target_link_libraries(labelprint_demo PRIVATE labelprint)
target_compile_options(labelprint_demo PRIVATE /utf-8)

# ============================================================================
# Tests
# ============================================================================
add_executable(test_runner
    test/test_main.cpp
    test/test_document.cpp
    test/test_backends.cpp
)
target_link_libraries(test_runner PRIVATE labelprint)
if(MSVC)
    target_compile_options(test_runner PRIVATE /utf-8)
endif()
