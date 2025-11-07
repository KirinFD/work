# CMakeList.txt for HAL
#   Build a library (`hal`) which exposes the header files as "hal/*.h"
#   Use header as: #include "hal/button.h"

cmake_minimum_required(VERSION 3.13)
project(hal C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Your sources
file(GLOB MY_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

add_library(hal STATIC ${MY_SOURCES})

# Public includes for consumers of hal (your app)
target_include_directories(hal PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Prefer pkg-config to find libgpiod v2
include(CheckIncludeFile)
find_package(PkgConfig QUIET)

set(_linked_gpiod FALSE)

if(PKG_CONFIG_FOUND)
  pkg_check_modules(LIBGPIOD QUIET IMPORTED_TARGET libgpiod)
  if(TARGET PkgConfig::LIBGPIOD)
    # Optional: ensure v2+
    if(DEFINED LIBGPIOD_VERSION AND LIBGPIOD_VERSION VERSION_LESS "2.0.0")
      message(FATAL_ERROR "libgpiod >= 2.0.0 required, found ${LIBGPIOD_VERSION}")
    endif()

    target_link_libraries(hal PUBLIC PkgConfig::LIBGPIOD)
    set(_linked_gpiod TRUE)
    message(STATUS "Using libgpiod via pkg-config (version: ${LIBGPIOD_VERSION})")
  endif()
endif()

# Fallback manual discovery if pkg-config isn’t available or didn’t provide a target
if(NOT _linked_gpiod)
  find_path(GPIOD_INCLUDE_DIR gpiod.h
    PATHS /usr/local/include /usr/include
  )
  find_library(GPIOD_LIBRARY gpiod
    PATHS /usr/local/lib /usr/lib /usr/lib64
  )
  if(NOT GPIOD_INCLUDE_DIR OR NOT GPIOD_LIBRARY)
    message(FATAL_ERROR "libgpiod not found. Install libgpiod v2 or set PKG_CONFIG_PATH to where libgpiod.pc is located.")
  endif()

  # Add headers so consumers can include <gpiod.h> if they parse bias opts, etc.
  target_include_directories(hal PUBLIC ${GPIOD_INCLUDE_DIR})
  target_link_libraries(hal PUBLIC ${GPIOD_LIBRARY})
  message(STATUS "Using libgpiod via manual discovery: inc=${GPIOD_INCLUDE_DIR}, lib=${GPIOD_LIBRARY}")
endif()

# Threads (pthread)
find_package(Threads REQUIRED)
target_link_libraries(hal PUBLIC Threads::Threads)