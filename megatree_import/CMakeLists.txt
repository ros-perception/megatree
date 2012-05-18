cmake_minimum_required(VERSION 2.4.6)
include($ENV{ROS_ROOT}/core/rosbuild/rosbuild.cmake)

# Set the build type.  Options are:
#  Coverage       : w/ debug symbols, w/o optimization, w/ code-coverage
#  Debug          : w/ debug symbols, w/o optimization
#  Release        : w/o debug symbols, w/ optimization
#  RelWithDebInfo : w/ debug symbols, w/ optimization
#  MinSizeRel     : w/o debug symbols, w/ optimization, stripped binaries
#set(ROS_BUILD_TYPE RelWithDebInfo)

rosbuild_init()

#set the default path for built executables to the "bin" directory
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
#set the default path for built libraries to the "lib" directory
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

#uncomment if you have defined messages
#rosbuild_genmsg()
#uncomment if you have defined services
#rosbuild_gensrv()

#common commands for building c++ executables and libraries
#rosbuild_add_library(${PROJECT_NAME} src/example.cpp)
#target_link_libraries(${PROJECT_NAME} another_library)
#rosbuild_add_boost_directories()
#rosbuild_link_boost(${PROJECT_NAME} thread)
#rosbuild_add_executable(example examples/example.cpp)
#target_link_libraries(example ${PROJECT_NAME})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})
find_package(LibLAS)
if (LIBLAS_FOUND)
  rosbuild_add_executable(import_las src/import_las.cpp)
  target_link_libraries(import_las boost_filesystem)
  target_link_libraries(import_las ${LIBLAS_LIBRARY})
  target_link_libraries(import_las megatree)

  rosbuild_add_executable(sort_las src/sort_las.cpp)
  target_link_libraries(sort_las boost_filesystem)
  target_link_libraries(sort_las ${LIBLAS_LIBRARY})
  target_link_libraries(sort_las megatree)
endif (LIBLAS_FOUND)

rosbuild_add_executable(import_pts src/import_pts.cpp)
target_link_libraries(import_pts boost_filesystem)
target_link_libraries(import_pts megatree)

