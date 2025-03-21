cmake_minimum_required(VERSION 3.26)
project(JSLikePromise VERSION 0.1.0 LANGUAGES CXX)

include(CTest)
include(FetchContent)
include(GoogleTest)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.15.2
)
set(BUILD_GMOCK OFF CACHE BOOL "")
# # For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_library(JSLikePromise INTERFACE)
target_compile_features(JSLikePromise INTERFACE cxx_std_20)
target_include_directories(JSLikePromise INTERFACE ".")
target_sources(JSLikePromise
  INTERFACE FILE_SET "headers" TYPE HEADERS BASE_DIRS "." FILES
    "JSLikeBasePromise.hpp"
    "JSLikePromise.hpp"
    "JSLikePromiseAll.hpp"
    "JSLikePromiseAny.hpp"
)

add_executable(JSLikePromiseTest)
target_compile_features(JSLikePromiseTest PRIVATE cxx_std_20)
target_link_libraries(JSLikePromiseTest PRIVATE JSLikePromise gtest_main)
gtest_discover_tests(JSLikePromiseTest)
target_sources(JSLikePromiseTest
  PRIVATE FILE_SET "private" TYPE HEADERS BASE_DIRS "Test" FILES
    "Test/TranscriptionCounter.hpp"
  PRIVATE
    "Test/TestJsLikePromiseAll.cpp"
    "Test/TestJsLikePromiseAny.cpp"
    "Test/TestJsLikeValuedPromise.cpp"
    "Test/TestJsLikeVoidPromise.cpp"
    "Test/TestReadmeCodeSnippets.cpp"
)

add_executable(JSLikePromiseExampleCpp)
target_compile_features(JSLikePromiseExampleCpp PRIVATE cxx_std_20)
target_link_libraries(JSLikePromiseExampleCpp PRIVATE JSLikePromise)
target_sources(JSLikePromiseExampleCpp
  PRIVATE FILE_SET "private" TYPE HEADERS BASE_DIRS "Examples_C++20" FILES
    "Examples_C++20/difference01.hpp"
    "Examples_C++20/example01.hpp"
    "Examples_C++20/example02.hpp"
    "Examples_C++20/example02a.hpp"
    "Examples_C++20/example03.hpp"
    "Examples_C++20/example04.hpp"
    "Examples_C++20/example05.hpp"
    "Examples_C++20/example06.hpp"
    "Examples_C++20/example07.hpp"
  PRIVATE
    "Examples_C++20/examples.cpp"
)

add_custom_target(JSLikePromiseExampleJavascript
  SOURCES
    "Examples_JavaScript/difference01.js"
    "Examples_JavaScript/example01.js"
    "Examples_JavaScript/example02.js"
    "Examples_JavaScript/example02a.js"
    "Examples_JavaScript/example03.js"
    "Examples_JavaScript/example04.js"
    "Examples_JavaScript/example05.js"
    "Examples_JavaScript/example06.js"
    "Examples_JavaScript/example07.js"
)

add_custom_target(NodeDependency
  COMMAND npm install
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/mdautogen"
  SOURCES
    "mdautogen/package.json"
)

# Currently the generation script is only available on Windows
if(NOT WIN32)
  return()
endif()
add_custom_target(GenerateReadme
  COMMAND run.bat
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/mdautogen"
  SOURCES
    "mdautogen/mdautogen.js"
    "mdautogen/run.bat"
)
add_dependencies(GenerateReadme NodeDependency)
