cmake_minimum_required(VERSION 3.10)

project(ThreeVideoStream LANGUAGES C)

add_compile_options(-Wall -Wextra -pedantic -Werror -Wno-unused-parameter)

# for LSP / clang-tidy etc.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set (CMAKE_C_STANDARD 99)
SET (CMAKE_C_FLAGS_DEBUG "-g -O0")

# for sanitizer build
# set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0 -fno-omit-frame-pointer -fsanitize=address")
# set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")

# Glib
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTLIBS REQUIRED
  gobject-2.0
  glib-2.0
  gstreamer-1.0)

# add extra include directories
include_directories(
  /usr/lib/x86_64-linux-gnu/glib-2.0/include
  /usr/include/glib-2.0
  /usr/include/gstreamer-1.0
  )

link_libraries(gstreamer-1.0
  gobject-2.0
  glib-2.0)

link_directories(${GSTLIBS_LIBRARY_DIRS})

set(SOURCE_FILES main.c three_video_stream.h three_video_stream.c gst_helpers.h gst_helpers.c)

add_executable(ThreeVideoStream ${SOURCE_FILES})

target_link_libraries(ThreeVideoStream  ${ThreeVideoStream_LIBRARIES})
