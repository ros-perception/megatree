cmake_minimum_required(VERSION 2.4.6)
include($ENV{ROS_ROOT}/core/rosbuild/rosbuild.cmake)
  
# Set the build type.  Options are:
#  Coverage       : w/ debug symbols, w/o optimization, w/ code-coverage
#  Debug          : w/ debug symbols, w/o optimization
#  Release        : w/o debug symbols, w/ optimization
#  RelWithDebInfo : w/ debug symbols, w/ optimization
#  MinSizeRel     : w/o debug symbols, w/ optimization, stripped binaries
#set(ROS_BUILD_TYPE Debug)
set(ROS_BUILD_TYPE RelWithDebInfo)
#set(ROS_BUILD_TYPE Release)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})

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

# un-comment to start using hbase. You need to install Thrift and Hbase for this to work
#set(USE_HBASE 1)


if (USE_HBASE)
  set(CMAKE_CXX_FLAGS "-DUSE_HBASE")
  find_package(Thrift)
  include_directories(${THRIFT_INCLUDE_DIRS})
  include_directories(${PROJECT_SOURCE_DIR}/thrift_gen)
ENDIF (USE_HBASE)

if (USE_HBASE)
rosbuild_add_library(megatree_storage
  thrift_gen/Hbase_constants.cpp
  thrift_gen/Hbase_types.cpp
  thrift_gen/Hbase.cpp
  src/hbase_storage.cpp
  )
ENDIF (USE_HBASE)

rosbuild_add_library(megatree_storage
  src/disk_storage.cpp
  src/viz_storage.cpp
  src/storage_factory.cpp
  )

target_link_libraries(megatree_storage boost_iostreams boost_filesystem boost_system boost_program_options)
if (USE_HBASE)
  target_link_libraries(megatree_storage thrift)
ENDIF (USE_HBASE)

#target_link_libraries(megatree_storage profiler)  # google-perftools profiler
#target_link_libraries(megatree_storage tcmalloc)  # google-perftools tcmalloc

rosbuild_add_executable(bin/streaming_server src/streaming_server.cpp)
target_link_libraries(bin/streaming_server boost_filesystem boost_iostreams)
target_link_libraries(bin/streaming_server megatree_storage)

rosbuild_add_executable(bin/write src/write.cpp)
target_link_libraries(bin/write megatree_storage)

rosbuild_add_executable(bin/read src/read.cpp)
target_link_libraries(bin/read megatree_storage)

