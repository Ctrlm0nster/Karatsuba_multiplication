cmake_minimum_required(VERSION 3.16)

project(karatsuba
    VERSION 1.0.0
    DESCRIPTION "High-performance Karatsuba and Schoolbook integer multiplication with GMP"
    HOMEPAGE_URL "https://github.com/Ctrlm0nster/Karatsuba_multiplication"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build Options
option(BUILD_TESTING "Build the unit test suite" ON)
option(KARATSUBA_BUILD_CLI "Build the interactive CLI calculator" ON)
option(KARATSUBA_BUILD_BENCHMARKS "Build the benchmark executable" ON)
option(KARATSUBA_ENABLE_WARNINGS "Enable strict compiler warnings" ON)
option(KARATSUBA_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(KARATSUBA_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(KARATSUBA_ENABLE_TSAN "Enable ThreadSanitizer" OFF)

# Find Threads library for multi-threading
find_package(Threads REQUIRED)

# Find GMP and GMPXX libraries
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(GMPXX IMPORTED_TARGET gmpxx>=6.0 gmp>=6.0)
endif()

if(NOT TARGET PkgConfig::GMPXX)
    find_path(GMP_INCLUDE_DIR NAMES gmp.h gmpxx.h)
    find_library(GMP_LIB NAMES gmp)
    find_library(GMPXX_LIB NAMES gmpxx)
    if(GMP_INCLUDE_DIR AND GMP_LIB AND GMPXX_LIB)
        add_library(PkgConfig::GMPXX UNKNOWN IMPORTED)
        set_target_properties(PkgConfig::GMPXX PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${GMP_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES "${GMPXX_LIB};${GMP_LIB}"
        )
    else()
        message(FATAL_ERROR "GMP and GMPXX libraries are required to build this project.")
    endif()
endif()

# Compiler warning flags helper
function(karatsuba_apply_warnings target_name)
    if(KARATSUBA_ENABLE_WARNINGS)
        if(MSVC)
            target_compile_options(${target_name} PRIVATE /W4 /permissive-)
        else()
            target_compile_options(${target_name} PRIVATE
                -Wall
                -Wextra
                -Wpedantic
                -Wshadow
                -Wnon-virtual-dtor
                -Wunused
            )
        endif()
    endif()

    if(KARATSUBA_ENABLE_ASAN AND NOT MSVC)
        target_compile_options(${target_name} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(${target_name} PRIVATE -fsanitize=address)
    endif()

    if(KARATSUBA_ENABLE_UBSAN AND NOT MSVC)
        target_compile_options(${target_name} PRIVATE -fsanitize=undefined)
        target_link_options(${target_name} PRIVATE -fsanitize=undefined)
    endif()

    if(KARATSUBA_ENABLE_TSAN AND NOT MSVC)
        target_compile_options(${target_name} PRIVATE -fsanitize=thread -fno-omit-frame-pointer)
        target_link_options(${target_name} PRIVATE -fsanitize=thread)
    endif()
endfunction()

# -----------------------------------------------------------------------------
# Core Library Target
# -----------------------------------------------------------------------------
add_library(karatsuba_core
    src/karatsuba.cpp
    src/toom3.cpp
    src/schoolbook.cpp
    src/parser.cpp
    src/timer.cpp
)

add_library(karatsuba::core ALIAS karatsuba_core)

target_include_directories(karatsuba_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(karatsuba_core
    PUBLIC
        PkgConfig::GMPXX
        Threads::Threads
)

karatsuba_apply_warnings(karatsuba_core)

# -----------------------------------------------------------------------------
# CLI Application Target
# -----------------------------------------------------------------------------
if(KARATSUBA_BUILD_CLI)
    add_executable(karatsuba_cli
        apps/karatsuba_cli.cpp
    )
    target_link_libraries(karatsuba_cli PRIVATE karatsuba_core)
    karatsuba_apply_warnings(karatsuba_cli)
endif()

# -----------------------------------------------------------------------------
# Benchmark Executable Target
# -----------------------------------------------------------------------------
if(KARATSUBA_BUILD_BENCHMARKS)
    add_executable(karatsuba_benchmark
        benchmarks/benchmark.cpp
    )
    target_link_libraries(karatsuba_benchmark PRIVATE karatsuba_core)
    karatsuba_apply_warnings(karatsuba_benchmark)
endif()

# -----------------------------------------------------------------------------
# Unit Testing Target
# -----------------------------------------------------------------------------
if(BUILD_TESTING)
    include(CTest)
    add_executable(karatsuba_tests
        tests/test_main.cpp
        tests/test_schoolbook.cpp
        tests/test_karatsuba.cpp
        tests/test_toom3.cpp
        tests/test_concurrency.cpp
        tests/test_parser.cpp
        tests/test_timer.cpp
    )
    target_link_libraries(karatsuba_tests PRIVATE karatsuba_core)
    target_include_directories(karatsuba_tests PRIVATE tests)
    karatsuba_apply_warnings(karatsuba_tests)

    add_test(NAME unit_tests COMMAND karatsuba_tests)
endif()

# -----------------------------------------------------------------------------
# Installation Rules
# -----------------------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS karatsuba_core
    EXPORT karatsubaTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if(KARATSUBA_BUILD_CLI)
    install(TARGETS karatsuba_cli RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if(KARATSUBA_BUILD_BENCHMARKS)
    install(TARGETS karatsuba_benchmark RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

install(DIRECTORY include/karatsuba
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
