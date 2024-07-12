cmake_minimum_required(VERSION 3.16)
set(PROJECT_VERSION_MINOR 2)
project(xiclops LANGUAGES C)

set(CMAKE_C_COMPILER "cc")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(
    CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/
)

set(CMAKE_CXX_FLAGS "-Wall" "-Wextra" "-ggdb")
include_directories(
    src
	include
	/usr/include/
    /opt/XIMEA/include
    raylib/raylib-5.0_linux_amd64/include
)


file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_SOURCE_DIR} "src/*.c")

link_directories(${CMAKE_SOURCE_DIR}/raylib/raylib-5.0_linux_amd64/lib)

add_executable(${PROJECT_NAME}
  ${SOURCES}
)

target_link_libraries(${PROJECT_NAME}
  ${LIBS}
  dl
  m
  m3api
  pthread
  raylib
 )
