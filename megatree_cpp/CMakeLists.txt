cmake_minimum_required(VERSION 2.4.6)
include($ENV{ROS_ROOT}/core/rosbuild/rosbuild.cmake)
  
# Set the build type.  Options are:
#  Coverage       : w/ debug symbols, w/o optimization, w/ code-coverage
#  Debug          : w/ debug symbols, w/o optimization
#  Release        : w/o debug symbols, w/ optimization
#  RelWithDebInfo : w/ debug symbols, w/ optimization
#  MinSizeRel     : w/o debug symbols, w/ optimization, stripped binaries
set(ROS_BUILD_TYPE Debug)
#set(ROS_BUILD_TYPE RelWithDebInfo)
#set(ROS_BUILD_TYPE Release)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})

find_package(Eigen REQUIRED)
include_directories(${Eigen_INCLUDE_DIRS})

rosbuild_init()

#set the default path for built executables to the "bin" directory
#set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
#set the default path for built libraries to the "lib" directory
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

#uncomment if you have defined messages
#rosbuild_genmsg()
#uncomment if you have defined services
#rosbuild_gensrv()

add_definitions(-Wno-missing-field-initializers)  # For argp_parse

rosbuild_add_library(megatree
  src/node_file.cpp
  src/node_handle.cpp
  src/node.cpp
  src/long_id.cpp
  src/megatree.cpp
  src/tree_functions.cpp
  )

#target_link_libraries(megatree profiler)  # google-perftools profiler
#target_link_libraries(megatree tcmalloc)  # google-perftools tcmalloc

rosbuild_add_executable(bin/memtest src/memtest.cpp)
target_link_libraries(bin/memtest boost_filesystem boost_iostreams)
target_link_libraries(bin/memtest megatree)

rosbuild_add_executable(bin/bind src/bind.cpp)
target_link_libraries(bin/bind boost_filesystem boost_iostreams)
target_link_libraries(bin/bind megatree)

rosbuild_add_executable(bin/listtest src/listtest.cpp)
target_link_libraries(bin/listtest boost_filesystem boost_iostreams)
target_link_libraries(bin/listtest megatree)

rosbuild_add_executable(bin/create src/create.cpp)
target_link_libraries(bin/create boost_filesystem)
target_link_libraries(bin/create megatree)

rosbuild_add_executable(bin/analyze src/analyze.cpp)
target_link_libraries(bin/analyze boost_filesystem boost_system)
target_link_libraries(bin/analyze megatree)


rosbuild_add_executable(bin/benchmark_write src/benchmark_write.cpp)
target_link_libraries(bin/benchmark_write boost_filesystem boost_iostreams)
target_link_libraries(bin/benchmark_write megatree)


rosbuild_add_executable(bin/benchmark_read src/benchmark_read.cpp)
target_link_libraries(bin/benchmark_read boost_filesystem boost_iostreams)
target_link_libraries(bin/benchmark_read megatree)

rosbuild_add_gtest(test/test_basics test/test_basics.cpp)
target_link_libraries(test/test_basics boost_filesystem boost_iostreams)
target_link_libraries(test/test_basics megatree)

rosbuild_add_gtest(test/test_list test/test_list.cpp)
target_link_libraries(test/test_list boost_filesystem boost_iostreams)
target_link_libraries(test/test_list megatree)

rosbuild_add_executable(bin/fixup_root src/fixup_root.cpp)
target_link_libraries(bin/fixup_root boost_filesystem boost_iostreams)
target_link_libraries(bin/fixup_root megatree)

rosbuild_add_gtest(test/test_long_id test/test_long_id.cpp)
target_link_libraries(test/test_long_id boost_filesystem boost_iostreams)
target_link_libraries(test/test_long_id megatree)

rosbuild_add_executable(bin/repair_from_children src/repair_from_children.cpp)
target_link_libraries(bin/repair_from_children megatree)

#common commands for building c++ executables and libraries
#target_link_libraries(${PROJECT_NAME} another_library)
#rosbuild_add_boost_directories()
#rosbuild_link_boost(${PROJECT_NAME} thread)
#target_link_libraries(example ${PROJECT_NAME})
