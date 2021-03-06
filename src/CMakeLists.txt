cmake_minimum_required(VERSION 2.8.2 FATAL_ERROR)
set(CMAKE_LEGACY_CYGWIN_WIN32 0)

project("Devcash" C CXX)

set (CMAKE_CXX_STANDARD 14)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (DevCash_VERSION_MAJOR 0)
set (DevCash_VERSION_MINOR 2)

enable_testing()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR
    "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(warnings "-Wall -Wextra \
                  -pedantic \
                  -Wlogical-op \
                  -Wno-unused-function \
                  -Wno-unused-parameter \
                  -Wno-vla \
                  -fdiagnostics-show-option")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(warnings "/W4 /WX /EHsc")
endif()

if (NOT CONFIGURED_ONCE)
    set(CMAKE_CXX_FLAGS "${warnings} -Woverloaded-virtual -Werror -std=gnu++11"
        CACHE STRING "Flags used by the compiler during all build types." FORCE)
    set(CMAKE_C_FLAGS   "${warnings}"
        CACHE STRING "Flags used by the compiler during all build types." FORCE)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__FILENAME__='\"$(subst ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"'")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_BINARY_DIR})

# External third-party libraries
find_library(ZMQ zmq)
find_library(PQXX pqxx)

### protobuf defs ###
find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS pbuf/devv.proto)
#protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS EXPORT_MACRO DLL_EXPORT foo.proto)
protobuf_generate_python(PROTO_PY pbuf/devv.proto)
#add_executable(bar bar.cc ${PROTO_SRCS} ${PROTO_HDRS})

# Dummy target that triggers python protobuf generation
add_custom_target(myTarget ALL DEPENDS ${PROTO_PY})

file(GLOB_RECURSE dcSources "*.cpp")
file(GLOB_RECURSE dcHeaders "*.h*")
set (dcInclude "")
foreach (_headerFile ${dcHeaders})
  get_filename_component(_dir ${_headerFile} PATH)
  list (APPEND dcInclude ${_dir})
endforeach()
list(REMOVE_DUPLICATES dcInclude)

#Protobuf generated library
ADD_LIBRARY(devvpbuf devv.pb.h devv.pb.cc)
set_source_files_properties(devv.pb.h devv.pb.cc PROPERTIES GENERATED TRUE)

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

# Build the primitives module objects
file(GLOB primitives_SRC
    "primitives/*.cpp"
)
add_library(primitives
  ${primitives_SRC}
  )

# Add BlockchainModule.cpp
add_library(devcash_node modules/BlockchainModule.cpp)

set(devcash_core_libs devcash_node common concurrency consensus io primitives devvpbuf)

option(WITH_THREAD_TESTING "Enable thread testing" OFF)

if(WITH_THREAD_TESTING)
  add_definitions(-DTEST_THREADS)
endif()

# Find OpenSSL and setup variables
find_package (OpenSSL REQUIRED)

# Find thread library and setup variables
find_package(Threads REQUIRED)

# Find boost and setup variables
find_package(Boost COMPONENTS date_time filesystem thread
  system program_options log REQUIRED)

set(CONFIGURED_ONCE TRUE CACHE INTERNAL
    "A flag showing that CMake has configured at least once.")

# Convenience variable containing all linker libs
set(devv_all_libs
        ${devcash_core_libs}
        ${ZMQ}
        ${PROTOBUF_LIBRARY}
        ${OPENSSL_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        ${Boost_LIBRARIES}
        ${PQXX}
        )

#
# devcash executable
#
add_executable(devcash devcash.cpp)
target_link_libraries (devcash ${devv_all_libs})
target_include_directories(devcash PRIVATE ${dcInclude})
install (TARGETS devcash DESTINATION bin)

#
# scanner
#
add_executable(scanner scanner.cpp ${dcInclude})
target_link_libraries (scanner ${devv_all_libs})
target_include_directories(scanner PRIVATE ${dcInclude})
install (TARGETS scanner DESTINATION bin)

#
# devv-sign
#
add_executable(devv-sign devv-sign.cpp ${dcInclude} ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries (devv-sign ${devv_all_libs})
target_include_directories(devv-sign PRIVATE ${dcInclude})
install (TARGETS devv-sign DESTINATION bin)

#
# annnouncer
#
add_executable(announcer announcer.cpp ${dcInclude} ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries (announcer ${devv_all_libs})
target_include_directories(announcer PRIVATE ${dcInclude})
install (TARGETS announcer DESTINATION bin)

#
# pb_annnouncer
#
add_executable(pb_announcer pb_announcer.cpp ${dcInclude} ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries (pb_announcer ${devv_all_libs})
target_include_directories(pb_announcer PRIVATE ${dcInclude})
install (TARGETS pb_announcer DESTINATION bin)

#
# repeater
#
add_executable(repeater repeater.cpp ${dcInclude})
target_link_libraries (repeater ${devv_all_libs})
target_include_directories(repeater PRIVATE ${dcInclude})
install (TARGETS repeater DESTINATION bin)

#
# db_repeater
#
add_executable(db_repeater db_repeater.cpp ${dcInclude})
target_link_libraries (db_repeater ${devv_all_libs})
target_include_directories(db_repeater PRIVATE ${dcInclude})
install (TARGETS db_repeater DESTINATION bin)

#
# circuit
#
add_executable(circuit circuit.cpp ${dcInclude})
target_link_libraries (circuit ${devv_all_libs})
target_include_directories(circuit PRIVATE ${dcInclude})
install (TARGETS circuit DESTINATION bin)

#
# laminar
#
add_executable(laminar laminar.cpp ${dcInclude})
target_link_libraries (laminar ${devv_all_libs})
target_include_directories(laminar PRIVATE ${dcInclude})
install (TARGETS laminar DESTINATION bin)

#
# turbulent
#
add_executable(turbulent turbulent.cpp ${dcInclude})
target_link_libraries (turbulent ${devv_all_libs})
target_include_directories(turbulent PRIVATE ${dcInclude})
install (TARGETS turbulent DESTINATION bin)

#
# verifier
#
add_executable(verifier verifier.cpp ${dcInclude})
target_link_libraries (verifier ${devv_all_libs})
target_include_directories(verifier PRIVATE ${dcInclude})
install (TARGETS verifier DESTINATION bin)

#
# i/o Tests
#
add_executable(ring_queue_test
  tests/concurrency/ring_queue_test.cpp
  )

target_link_libraries(ring_queue_test
  ${devcash_core_libs}
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
  ${devcash_core_libs}
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
  ${devcash_core_libs}
  ${CMAKE_THREAD_LIBS_INIT}
  ${Boost_LIBRARIES}
  )

install (TARGETS boost_spsc_test DESTINATION bin)

# # # io_server_test
add_executable(io_server_test
  tests/io/transaction_server.cpp
  )

target_link_libraries(io_server_test
  ${devcash_core_libs}
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
  ${devcash_core_libs}
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
include(${CMAKE_CURRENT_SOURCE_DIR}/gtest/CMakeLists.txt)
