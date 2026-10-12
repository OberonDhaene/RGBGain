cmake_minimum_required(VERSION 3.15)
project(RGBGain CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Path to a clone of https://github.com/AcademySoftwareFoundation/openfx
set(OFX_ROOT "" CACHE PATH "Path to the openfx repository")
if(NOT OFX_ROOT)
  message(FATAL_ERROR "Set -DOFX_ROOT=/path/to/openfx")
endif()

file(GLOB OFX_SUPPORT_SRC "${OFX_ROOT}/Support/Library/*.cpp")

add_library(RGBGain MODULE RGBGain.cpp ${OFX_SUPPORT_SRC})

target_include_directories(RGBGain PRIVATE
  "${OFX_ROOT}/include"
  "${OFX_ROOT}/Support/include"
  "${OFX_ROOT}/Support/Plugins/include")

# Bundle layout: RGBGain.ofx.bundle/Contents/<Arch>/RGBGain.ofx
if(WIN32)
  set(OFX_ARCH "Win64")
elseif(APPLE)
  set(OFX_ARCH "MacOS")
else()
  set(OFX_ARCH "Linux-x86-64")
endif()

set(BUNDLE_DIR "${CMAKE_BINARY_DIR}/RGBGain.ofx.bundle/Contents/${OFX_ARCH}")

set_target_properties(RGBGain PROPERTIES
  PREFIX ""
  SUFFIX ".ofx"
  LIBRARY_OUTPUT_DIRECTORY "${BUNDLE_DIR}"
  RUNTIME_OUTPUT_DIRECTORY "${BUNDLE_DIR}"
  LIBRARY_OUTPUT_DIRECTORY_RELEASE "${BUNDLE_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_RELEASE "${BUNDLE_DIR}")

if(APPLE)
  # Build for both Intel and Apple Silicon
  set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
endif()
