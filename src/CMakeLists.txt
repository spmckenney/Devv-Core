cmake_minimum_required(VERSION 2.8.2 FATAL_ERROR)
set(CMAKE_LEGACY_CYGWIN_WIN32 0)

project("Devcash")

set (CMAKE_CXX_STANDARD 14)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (DevCash_VERSION_MAJOR 0)
set (DevCash_VERSION_MINOR 2)

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

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__FILENAME__='\"$(subst ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"'")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_BINARY_DIR})

file(GLOB_RECURSE dcSources "*.cpp")
file(GLOB_RECURSE dcHeaders "*.h*")
set (dcInclude "")
foreach (_headerFile ${dcHeaders})
  get_filename_component(_dir ${_headerFile} PATH)
  list (APPEND dcInclude ${_dir})
endforeach()
list(REMOVE_DUPLICATES dcInclude)

# External third-party libraries
find_library(ZMQ zmq)

# Build the common module objects
file(GLOB common_SRC
    "common/*.cpp" "common/*.c"
)
add_library(common ${common_SRC})

# Build the concurrency module objects
file(GLOB concurrency_SRC
    "concurrency/*.cpp"
)
add_library(concurrency ${concurrency_SRC})

# Build the consensus module objects
file(GLOB consensus_SRC
    "consensus/*.cpp"
)
add_library(consensus ${consensus_SRC})

# Build the io module objects
file(GLOB io_SRC
    "io/*.cpp"
)
add_library(io
  ${io_SRC}
  )

# Add DevcashNode.cpp
add_library(DevcashNode node/DevcashNode.cpp)

option(WITH_THREAD_TESTING "Enable thread testing" OFF)

if(WITH_THREAD_TESTING)
  add_definitions(-DTEST_THREADS)
endif()

add_executable(devcash devcash.cpp ${dcInclude})
target_link_libraries (devcash
  DevcashNode
  common
  io
  concurrency
  consensus
  ${ZMQ}
  )

find_package (OpenSSL REQUIRED)
if (OPENSSL_FOUND)
  include_directories(${OPENSSL_INCLUDE_DIR})
  target_link_libraries (devcash ${OPENSSL_LIBRARIES})
endif (OPENSSL_FOUND)

find_package(Threads)
target_link_libraries(devcash ${CMAKE_THREAD_LIBS_INIT})

find_package(Boost COMPONENTS date_time filesystem thread
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
# Scanner
#

add_executable(scanner scanner.cpp ${dcInclude})
target_link_libraries (scanner
  common
  concurrency
  consensus
  ${ZMQ}
  )

find_package (OpenSSL REQUIRED)
if (OPENSSL_FOUND)
  include_directories(${OPENSSL_INCLUDE_DIR})
  target_link_libraries (scanner ${OPENSSL_LIBRARIES})
endif (OPENSSL_FOUND)

find_package(Threads)
target_link_libraries(scanner ${CMAKE_THREAD_LIBS_INIT})

find_package(Boost COMPONENTS date_time filesystem thread
  system program_options log REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  target_link_libraries(scanner ${Boost_LIBRARIES})
endif(Boost_FOUND)

target_include_directories(scanner PRIVATE ${dcInclude})

install (TARGETS scanner DESTINATION bin)

#
# Annnouncer
#

add_executable(announcer announcer.cpp ${dcInclude})
target_link_libraries (announcer
  common
  concurrency
  consensus
  io
  ${ZMQ}
  )

find_package (OpenSSL REQUIRED)
if (OPENSSL_FOUND)
  include_directories(${OPENSSL_INCLUDE_DIR})
  target_link_libraries (announcer ${OPENSSL_LIBRARIES})
endif (OPENSSL_FOUND)

find_package(Threads)
target_link_libraries(announcer ${CMAKE_THREAD_LIBS_INIT})

find_package(Boost COMPONENTS date_time filesystem thread
  system program_options log REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  target_link_libraries(announcer ${Boost_LIBRARIES})
endif(Boost_FOUND)

target_include_directories(announcer PRIVATE ${dcInclude})

install (TARGETS announcer DESTINATION bin)

#
# Chain state Tests
#

add_executable(circuit circuit.cpp ${dcInclude})
target_link_libraries (circuit
  common
  concurrency
  consensus
  ${ZMQ}
  )

find_package (OpenSSL REQUIRED)
if (OPENSSL_FOUND)
  include_directories(${OPENSSL_INCLUDE_DIR})
  target_link_libraries (circuit ${OPENSSL_LIBRARIES})
endif (OPENSSL_FOUND)

find_package(Threads)
target_link_libraries(circuit ${CMAKE_THREAD_LIBS_INIT})

find_package(Boost COMPONENTS date_time filesystem thread
  system program_options log REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  target_link_libraries(circuit ${Boost_LIBRARIES})
endif(Boost_FOUND)

target_include_directories(circuit PRIVATE ${dcInclude})

install (TARGETS circuit DESTINATION bin)

add_executable(laminar laminar.cpp ${dcInclude})
target_link_libraries (laminar
  common
  concurrency
  consensus
  ${ZMQ}
  )

find_package (OpenSSL REQUIRED)
if (OPENSSL_FOUND)
  include_directories(${OPENSSL_INCLUDE_DIR})
  target_link_libraries (laminar ${OPENSSL_LIBRARIES})
endif (OPENSSL_FOUND)

find_package(Threads)
target_link_libraries(laminar ${CMAKE_THREAD_LIBS_INIT})

find_package(Boost COMPONENTS date_time filesystem thread
  system program_options log REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  target_link_libraries(laminar ${Boost_LIBRARIES})
endif(Boost_FOUND)

target_include_directories(laminar PRIVATE ${dcInclude})

install (TARGETS laminar DESTINATION bin)

add_executable(turbulent turbulent.cpp ${dcInclude})
target_link_libraries (turbulent
  common
  concurrency
  consensus
  ${ZMQ}
  )

find_package (OpenSSL REQUIRED)
if (OPENSSL_FOUND)
  include_directories(${OPENSSL_INCLUDE_DIR})
  target_link_libraries (turbulent ${OPENSSL_LIBRARIES})
endif (OPENSSL_FOUND)

find_package(Threads)
target_link_libraries(turbulent ${CMAKE_THREAD_LIBS_INIT})

find_package(Boost COMPONENTS date_time filesystem thread
  system program_options log REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  target_link_libraries(turbulent ${Boost_LIBRARIES})
endif(Boost_FOUND)

target_include_directories(turbulent PRIVATE ${dcInclude})

install (TARGETS turbulent DESTINATION bin)

add_executable(verifier verifier.cpp ${dcInclude})
target_link_libraries (verifier
  common
  concurrency
  consensus
  ${ZMQ}
  )

find_package (OpenSSL REQUIRED)
if (OPENSSL_FOUND)
  include_directories(${OPENSSL_INCLUDE_DIR})
  target_link_libraries (verifier ${OPENSSL_LIBRARIES})
endif (OPENSSL_FOUND)

find_package(Threads)
target_link_libraries(verifier ${CMAKE_THREAD_LIBS_INIT})

find_package(Boost COMPONENTS date_time filesystem thread
  system program_options log REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  target_link_libraries(verifier ${Boost_LIBRARIES})
endif(Boost_FOUND)

target_include_directories(verifier PRIVATE ${dcInclude})

install (TARGETS verifier DESTINATION bin)

#
# i/o Tests
#
add_executable(ring_queue_test
  tests/concurrency/ring_queue_test.cpp
  )

target_link_libraries(ring_queue_test
  common
  consensus
  ${OPENSSL_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${Boost_LIBRARIES}
  )

install (TARGETS ring_queue_test DESTINATION bin)

# # # devcash_spsc_queue_test
add_executable(devcash_spsc_queue_test
  tests/concurrency/devcash_spsc_queue_test.cpp
  )

target_link_libraries(devcash_spsc_queue_test
  common
  consensus
  ${OPENSSL_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${Boost_LIBRARIES}
  )

install (TARGETS devcash_spsc_queue_test DESTINATION bin)

# # # boost_spsc_test
add_executable(boost_spsc_test
  tests/concurrency/boost_spsc_test.cpp
  )

target_link_libraries(boost_spsc_test
  common
  ${CMAKE_THREAD_LIBS_INIT}
  ${Boost_LIBRARIES}
  )

install (TARGETS boost_spsc_test DESTINATION bin)

# # # io_server_test
add_executable(io_server_test
  tests/io/transaction_server.cpp
  )

target_link_libraries(io_server_test
  io
  common
  ${ZMQ}
  ${OPENSSL_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${Boost_LIBRARIES}
  )

# # # io_client_test
add_executable(io_client_test
  tests/io/transaction_client.cpp
  )

target_link_libraries(io_client_test
  io
  common
  ${ZMQ}
  ${OPENSSL_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${Boost_LIBRARIES}
  )

# # # zmq_envelope_publisher
add_executable(zmq_envelope_publisher
  tests/io/zmq_envelope_publisher.cpp
  )

target_link_libraries(zmq_envelope_publisher
  ${ZMQ}
  )

# # # zmq_envelope_subscriber
add_executable(zmq_envelope_subscriber
  tests/io/zmq_envelope_subscriber.cpp
  )

target_link_libraries(zmq_envelope_subscriber
  ${ZMQ}
  )


######### gtest ##########

# Download and unpack googletest at configure time
#configure_file(CMakeLists.txt.gtest googletest-download/CMakeLists.txt)
#execute_process(COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" .
#    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/googletest-download" )
#execute_process(COMMAND "${CMAKE_COMMAND}" --build .
#    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/googletest-download" )

# Prevent GoogleTest from overriding our compiler/linker options
# when building with Visual Studio
#set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This adds
# the following targets: gtest, gtest_main, gmock
# and gmock_main
#add_subdirectory("${CMAKE_BINARY_DIR}/googletest-src"
#                 "${CMAKE_BINARY_DIR}/googletest-build")

# The gtest/gmock targets carry header search path
# dependencies automatically when using CMake 2.8.11 or
# later. Otherwise we have to add them here ourselves.
#if(CMAKE_VERSION VERSION_LESS 2.8.11)
#    include_directories("${gtest_SOURCE_DIR}/include"
#                        "${gmock_SOURCE_DIR}/include")
#endif()

# Now simply link your own targets against gtest, gmock,
# etc. as appropriate
