find_path(LIBLAS_INCLUDE_DIR liblas/liblas.hpp)

find_library(LIBLAS_LIBRARY NAMES las)

set(LIBLAS_LIBRARIES ${LIBLAS_LIBRARY} )
set(LIBLAS_INCLUDE_DIRS ${LIBLAS_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  LibLAS DEFAULT_MSG
  LIBLAS_LIBRARY LIBLAS_INCLUDE_DIR)

mark_as_advanced(LIBLAS_INCLUDE_DIR LIBLAS_LIBRARY)

# http://www.vtk.org/Wiki/CMake:How_To_Find_Libraries