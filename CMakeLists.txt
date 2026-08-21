cmake_minimum_required(VERSION 3.20)
project(CyclomaticComplexity CXX)

# LLVM ships its own CMake package. find_package picks up the version that
# llvm-config resolves to; override with -DLLVM_DIR=/usr/lib/llvm-18/lib/cmake/llvm
find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

message(STATUS "Using LLVM ${LLVM_PACKAGE_VERSION} from ${LLVM_DIR}")

if(LLVM_PACKAGE_VERSION VERSION_LESS 15)
  message(FATAL_ERROR "LLVM 15 or newer required (found ${LLVM_PACKAGE_VERSION})")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(CyclomaticComplexity MODULE src/CyclomaticComplexity.cpp)

target_include_directories(CyclomaticComplexity SYSTEM PRIVATE
  ${LLVM_INCLUDE_DIRS}
  ${CLANG_INCLUDE_DIRS}
)

target_compile_definitions(CyclomaticComplexity PRIVATE ${LLVM_DEFINITIONS})

# LLVM is built without RTTI by default. A plugin compiled WITH RTTI will fail
# to link or crash at load time against such a build, so match whatever this
# LLVM was built with rather than assuming.
if(NOT LLVM_ENABLE_RTTI)
  target_compile_options(CyclomaticComplexity PRIVATE -fno-rtti)
endif()

# Deliberately NOT linking clangAST/clangBasic here. The plugin is dlopen'd
# into a clang process that already provides those symbols; linking them again
# duplicates LLVM's global state (command-line option registration in
# particular) and aborts at startup with "Option already exists".
if(APPLE)
  target_link_options(CyclomaticComplexity PRIVATE -undefined dynamic_lookup)
endif()

set_target_properties(CyclomaticComplexity PROPERTIES PREFIX "")
