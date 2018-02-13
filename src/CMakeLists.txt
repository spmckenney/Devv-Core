cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
set(CMAKE_LEGACY_CYGWIN_WIN32 0)

project("DevCash")

set (CMAKE_CXX_STANDARD 11)
set (DevCash_VERSION_MAJOR 0)
set (DevCash_VERSION_MINOR 1)

enable_testing()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR
    "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(warnings "-Wall -Wextra -std=gnu++11")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(warnings "/W4 /WX /EHsc")
endif()

if (NOT CONFIGURED_ONCE)
    set(CMAKE_CXX_FLAGS "${warnings}"
        CACHE STRING "Flags used by the compiler during all build types." FORCE)
    set(CMAKE_C_FLAGS   "${warnings}"
        CACHE STRING "Flags used by the compiler during all build types." FORCE)
endif()

#include_directories(${CMAKE_CURRENT_SOURCE_DIR})

#add_subdirectory()

file(GLOB_RECURSE dcSources "*.cpp")
file(GLOB_RECURSE dcHeaders "*.h*")
set (dcInclude "")
foreach (_headerFile ${dcHeaders})
        get_filename_component(_dir ${_headerFile} PATH)
        list (APPEND dcInclude ${_dir})
endforeach()
list(REMOVE_DUPLICATES dcInclude)

file(GLOB primitives_SRC
    "primitives/*.cpp"
)

find_package(OpenSSL REQUIRED)

add_library(primitives ${primitives_SRC})

add_library(init init.cpp)
add_library(util util.cpp utiltime.cpp utilstrencodings.cpp)

add_executable(DevCash devcash.cpp ${dcInclude})
target_link_libraries (DevCash
  init
  util
  primitives
  )

find_package (OpenSSL)
if (OPENSSL_FOUND)
  include_directories(${OPENSSL_INCLUDE_DIR})
  target_link_libraries (DevCash ${OPENSSL_LIBRARIES})
endif (OPENSSL_FOUND)

target_include_directories(DevCash PRIVATE ${dcInclude})

#add_test(toDoTest toDo)


set(CONFIGURED_ONCE TRUE CACHE INTERNAL
    "A flag showing that CMake has configured at least once.")
