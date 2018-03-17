cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
set(CMAKE_LEGACY_CYGWIN_WIN32 0)

project("Devcash")

set (CMAKE_CXX_STANDARD 11)
set (Devcash_VERSION_MAJOR 0)
set (Devcash_VERSION_MINOR 1)

enable_testing()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR
    "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(warnings "-Wall -Wextra -std=gnu++11 -Wno-unused-function")
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
include_directories(${CMAKE_SOURCE_DIR})

# External third-party libraries
find_library(ZMQ zmq)

# Facebook specific libraries
find_library(FOLLY folly PATHS)
find_library(ZSTD zstd)

# # # # Begin Thrift
find_program(THRIFT1 thrift1)
find_library(THRIFTCPP2 thriftcpp2)
find_library(THRIFTPROTOCOL thriftprotocol)
find_library(PROTOCOL protocol)
find_library(TRANSPORT transport)
find_path(THRIFT_COMPILER_INCLUDE thrift/templates)
set(THRIFT_TEMPLATES ${THRIFT_COMPILER_INCLUDE}/thrift/templates)
include(${THRIFT_COMPILER_INCLUDE}/thrift/ThriftLibrary.cmake)

#
# Generate thrift defs for C++ only. For python install via setuptools
#
set(THRIFT_DIR ${CMAKE_BINARY_DIR}/thrift)
file(MAKE_DIRECTORY ${THRIFT_DIR})
# # # # End Thrift

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

# Build the oracles module objects
file(GLOB oracles_SRC
    "oracles/*.cpp"
)
add_library(oracles ${oracles_SRC})

# Build the consensus module objects
file(GLOB consensus_SRC
    "consensus/*.cpp"
)
add_library(consensus ${consensus_SRC})

# Add init.cpp
add_library(init init.cpp)

option(WITH_THREAD_TESTING "Enable thread testing" OFF)

if(WITH_THREAD_TESTING)
  add_definitions(-DTEST_THREADS)
endif()

add_executable(devcash devcash.cpp ${dcInclude})
target_link_libraries (devcash
  init
  common
  primitives
  oracles
  consensus
  )

find_package (OpenSSL REQUIRED)
if (OPENSSL_FOUND)
  include_directories(${OPENSSL_INCLUDE_DIR})
  target_link_libraries (devcash ${OPENSSL_LIBRARIES})
endif (OPENSSL_FOUND)

find_package(Threads)
target_link_libraries(devcash ${CMAKE_THREAD_LIBS_INIT})

find_package(Boost COMPONENTS date_time filesystem
  system program_options log REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  target_link_libraries(devcash ${Boost_LIBRARIES})
endif(Boost_FOUND)

target_include_directories(devcash PRIVATE ${dcInclude})

set(CONFIGURED_ONCE TRUE CACHE INTERNAL
    "A flag showing that CMake has configured at least once.")

install (TARGETS devcash DESTINATION bin)

#
# i/o Tests
#
add_executable(ring_queue_test
  tests/queues/ring_queue_test.cpp
  )

target_link_libraries(ring_queue_test
  common
  init
  primitives
  oracles
  consensus
  ${OPENSSL_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${Boost_LIBRARIES}
  )

install (TARGETS ring_queue_test DESTINATION bin)

# # # devcash_spsc_queue_test
add_executable(devcash_spsc_queue_test
  tests/queues/devcash_spsc_queue_test.cpp
  )

target_link_libraries(devcash_spsc_queue_test
  common
  init
  primitives
  oracles
  consensus
  ${OPENSSL_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${Boost_LIBRARIES}
  )

install (TARGETS devcash_spsc_queue_test DESTINATION bin)

# # # boost_spsc_test
add_executable(boost_spsc_test
  tests/queues/boost_spsc_test.cpp
  )

target_link_libraries(boost_spsc_test
  common
  ${CMAKE_THREAD_LIBS_INIT}
  ${Boost_LIBRARIES}
  )

install (TARGETS boost_spsc_test DESTINATION bin)

#add_executable(producer_consumer_queue_test
#  tests/queues/producer_consumer_queue_test.cpp
#  )
#
#target_link_libraries(producer_consumer_queue_test
#  common
#  init
#  primitives
#  oracles
#  consensus
#  ${FOLLY}
#  ${OPENSSL_LIBRARIES}
#  ${CMAKE_THREAD_LIBS_INIT}
#  ${Boost_LIBRARIES}
#  )
#
