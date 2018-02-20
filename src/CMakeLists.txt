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

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE dcSources "*.cpp")
file(GLOB_RECURSE dcHeaders "*.h*")
set (dcInclude "")
foreach (_headerFile ${dcHeaders})
        get_filename_component(_dir ${_headerFile} PATH)
        list (APPEND dcInclude ${_dir})
endforeach()
list(REMOVE_DUPLICATES dcInclude)

# Build the primitives objects
file(GLOB primitives_SRC
    "primitives/*.cpp"
)
add_library(primitives ${primitives_SRC})

# Build the common module objects
file(GLOB common_SRC
    "common/*.cpp"
)
add_library(common ${common_SRC})

# Add init.cpp
add_library(init init.cpp)

option(WITH_THREAD_TESTING "Enable thread testing" OFF)

if(WITH_THREAD_TESTING)
  add_definitions(-DTEST_THREADS)
endif()

add_executable(DevCash devcash.cpp ${dcInclude})
target_link_libraries (DevCash
  init
  common
  primitives
  )

find_package (OpenSSL REQUIRED)
if (OPENSSL_FOUND)
  include_directories(${OPENSSL_INCLUDE_DIR})
  target_link_libraries (DevCash ${OPENSSL_LIBRARIES})
endif (OPENSSL_FOUND)

find_package(Threads)
target_link_libraries(DevCash ${CMAKE_THREAD_LIBS_INIT})

find_package(Boost COMPONENTS date_time filesystem system program_options log REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  target_link_libraries(DevCash ${Boost_LIBRARIES})
endif(Boost_FOUND)

target_include_directories(DevCash PRIVATE ${dcInclude})

set(CONFIGURED_ONCE TRUE CACHE INTERNAL
    "A flag showing that CMake has configured at least once.")
