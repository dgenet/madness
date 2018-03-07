#
# Generic Toolchain for MPI + MKL + TBB
#
# REQUIREMENTS:
# - in PATH:
#   mpicc and mpicxx
#
# OPTIONS:
# - environment variables:
#   * MKLROOT: the MKL root directory; if not set, will use /opt/intel/mkl
#   * TBBROOT: the TBB root directory; if not set, will use /opt/intel/tbb
#

# Compilers
set(CMAKE_C_COMPILER gcc)
set(CMAKE_Fortran_COMPILER gfortran)
set(CMAKE_CXX_COMPILER g++)
set(MPI_C_COMPILER mpicc)
set(MPI_Fortran_COMPILER mpif90)
set(MPI_CXX_COMPILER mpicxx)

# Compile flags
set(CMAKE_C_FLAGS_INIT             "-fPIC -std=c99" CACHE STRING "Inital C compile flags")
set(CMAKE_C_FLAGS_DEBUG            "-fPIC -g -Wall" CACHE STRING "Inital C debug compile flags")
set(CMAKE_C_FLAGS_MINSIZEREL       "-fPIC -Os -march=native -DNDEBUG" CACHE STRING "Inital C minimum size release compile flags")
set(CMAKE_C_FLAGS_RELEASE          "-fPIC -O3 -march=native -DNDEBUG" CACHE STRING "Inital C release compile flags")
set(CMAKE_C_FLAGS_RELWITHDEBINFO   "-fPIC -O2 -g -Wall" CACHE STRING "Inital C release with debug info compile flags")
set(CMAKE_CXX_FLAGS_INIT           "-fPIC -std=c++11" CACHE STRING "Inital C++ compile flags")
set(CMAKE_CXX_FLAGS_DEBUG          "-fPIC -g -Wall" CACHE STRING "Inital C++ debug compile flags")
set(CMAKE_CXX_FLAGS_MINSIZEREL     "-fPIC -Os -march=native -DNDEBUG" CACHE STRING "Inital C++ minimum size release compile flags")
set(CMAKE_CXX_FLAGS_RELEASE        "-fPIC -O3 -march=native -DNDEBUG" CACHE STRING "Inital C++ release compile flags")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-fPIC -O2 -g -Wall" CACHE STRING "Inital C++ release with debug info compile flags")

# Libraries
if(EXISTS $ENV{MKLROOT})
  set(MKL_ROOT_DIR "$ENV{MKLROOT}" CACHE PATH "MKL root directory")
else()
  set(MKL_ROOT_DIR "/opt/intel/mkl" CACHE PATH "MKL root directory")
endif()
if(EXISTS $ENV{TBBROOT})
  set(TBB_ROOT_DIR "$ENV{TBBROOT}" CACHE PATH "TBB root directory")
else()
  set(TBB_ROOT_DIR "/opt/intel/tbb" CACHE PATH "TBB root directory")
endif()

# Flags
set(BLAS_LINKER_FLAGS "-L${MKL_ROOT_DIR}/lib -lmkl_intel_lp64 -lmkl_core -lmkl_sequential -lm" CACHE STRING "BLAS linker flags")
set(LAPACK_LINKER_FLAGS "" CACHE STRING "LAPACK linker flags")
set(INTEGER4 TRUE CACHE BOOL "Set Fortran integer size to 4 bytes")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lpthread")