find_path(THRIFT_INCLUDE_DIR thrift/Thrift.h)
set(THRIFT_INCLUDE_DIR ${THRIFT_INCLUDE_DIR}/thrift)

find_library(THRIFT_LIBRARY NAMES thrift)

set(THRIFT_LIBRARIES ${THRIFT_LIBRARY} )
set(THRIFT_INCLUDE_DIRS ${THRIFT_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  Thrift DEFAULT_MSG
  THRIFT_LIBRARY THRIFT_INCLUDE_DIR)

mark_as_advanced(THRIFT_INCLUDE_DIR THRIFT_LIBRARY)

# http://www.vtk.org/Wiki/CMake:How_To_Find_Libraries