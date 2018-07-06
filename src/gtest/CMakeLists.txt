# gtest/CMakeLists.txt
#
#    Date: Jun 23 2018
#  Author: Shawn McKenney <shawn.mckenney@emmion.com>
#
# Enable testing, configure GoogleTest and GoogleMock
# and compile tests
#

enable_testing()
find_package(GTest REQUIRED)
#find_package(GMock REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Now simply link against gtest or gtest_main as needed. Eg
add_executable(primitives_test gtest/primitives_test.cpp)
target_link_libraries(primitives_test
  ${GTEST_BOTH_LIBRARIES}
  ${devcash_core_libs}
  ${ZMQ}
  ${OPENSSL_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${Boost_LIBRARIES})

# Now simply link against gtest or gtest_main as needed. Eg
add_executable(consensus_test gtest/consensus_test.cpp)
target_link_libraries(consensus_test
  ${GTEST_BOTH_LIBRARIES}
  ${devcash_core_libs}
  ${ZMQ}
  ${OPENSSL_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${Boost_LIBRARIES})

add_test(NAME primitives_test COMMAND primitives_test)
add_test(NAME consensus_test COMMAND consensus_test)
