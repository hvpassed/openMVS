# CMake powered build system for OpenMVS

########################################################################
#
# Project-wide settings
CMAKE_MINIMUM_REQUIRED(VERSION 3.24)
IF(POLICY CMP0011)
	CMAKE_POLICY(SET CMP0011 NEW)
ENDIF()
IF(POLICY CMP0072)
	CMAKE_POLICY(SET CMP0072 NEW)
ENDIF()
IF(POLICY CMP0074)
	CMAKE_POLICY(SET CMP0074 NEW)
ENDIF()
IF(POLICY CMP0104)
	CMAKE_POLICY(SET CMP0104 NEW)
ENDIF()
IF(POLICY CMP0146)
	CMAKE_POLICY(SET CMP0146 OLD) # not required, but forced by OpenCV old support for CUDA
ENDIF()
IF(POLICY CMP0144)
	CMAKE_POLICY(SET CMP0144 NEW)
ENDIF()
# List configuration options
SET(OpenMVS_BUILD_TOOLS ON CACHE BOOL "Build example applications")
SET(OpenMVS_USE_OPENMP ON CACHE BOOL "Enable OpenMP library")
SET(OpenMVS_USE_OPENGL ON CACHE BOOL "Enable OpenGL library")
SET(OpenMVS_USE_BREAKPAD ON CACHE BOOL "Enable BreakPad library")
SET(OpenMVS_USE_PYTHON ON CACHE BOOL "Enable Python library bindings")
SET(OpenMVS_USE_CERES OFF CACHE BOOL "Enable CERES optimization library")
SET(OpenMVS_USE_CUDA ON CACHE BOOL "Enable CUDA library")
SET(OpenMVS_USE_FAST_FLOAT2INT OFF CACHE BOOL "Use an optimized code to convert real numbers to int")
SET(OpenMVS_USE_FAST_INVSQRT OFF CACHE BOOL "Use an optimized code to compute the inverse square root (slower in fact on modern compilers)")
SET(OpenMVS_USE_FAST_CBRT OFF CACHE BOOL "Use an optimized code to compute the cubic root")
SET(OpenMVS_USE_SSE ON CACHE BOOL "Enable SSE optimizations")
SET(OpenMVS_MAX_CUDA_COMPATIBILITY OFF CACHE BOOL "Build for maximum CUDA device compatibility")
SET(OpenMVS_ENABLE_TESTS ON CACHE BOOL "Enable test code")

# Load automatically VCPKG toolchain if available
IF(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
	SET(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
	IF(NOT DEFINED VCPKG_TARGET_TRIPLET AND DEFINED ENV{VCPKG_DEFAULT_TRIPLET})
		SET(VCPKG_TARGET_TRIPLET "$ENV{VCPKG_DEFAULT_TRIPLET}" CACHE STRING "")
	ENDIF()
ENDIF()
IF(OpenMVS_USE_PYTHON)
	LIST(APPEND VCPKG_MANIFEST_FEATURES "python")
	SET(PARTIAL_BUILD_SHARED_LIBS ON)
ENDIF()
IF(OpenMVS_USE_CUDA)
	LIST(APPEND VCPKG_MANIFEST_FEATURES "cuda")
ENDIF()

# Name of the project.
#
# CMake files in this project can refer to the root source directory
# as ${OpenMVS_SOURCE_DIR} and to the root binary directory as
# ${OpenMVS_BINARY_DIR}.
PROJECT(OpenMVS LANGUAGES CXX)

SET(OpenMVS_MAJOR_VERSION 2)
SET(OpenMVS_MINOR_VERSION 3)
SET(OpenMVS_PATCH_VERSION 0)
SET(OpenMVS_VERSION ${OpenMVS_MAJOR_VERSION}.${OpenMVS_MINOR_VERSION}.${OpenMVS_PATCH_VERSION})

# Disable SSE on unsupported platforms
IF(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|ARM|aarch64|AARCH64)")
	SET(OpenMVS_USE_SSE OFF)
ENDIF()

# Define helper functions and macros.
INCLUDE(build/Utils.cmake)

# Init session with macros defined in Utils.cmake
GetOperatingSystemArchitectureBitness(SYSTEM)
ComposePackageLibSuffix()
ConfigCompilerAndLinker()
ConfigLibrary()

# Find dependencies:
SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/build/Modules)

# Find required packages
SET(OpenMVS_EXTRA_INCLUDES "")
SET(OpenMVS_EXTRA_LIBS "")

if(OpenMVS_USE_OPENMP)
	SET(OpenMP_LIBS "")
	FIND_PACKAGE(OpenMP)
	if(OPENMP_FOUND)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
		SET(_USE_OPENMP TRUE)
		#cmake only check for separate OpenMP library on AppleClang 7+
		#https://github.com/Kitware/CMake/blob/42212f7539040139ecec092547b7d58ef12a4d72/Modules/FindOpenMP.cmake#L252
		if (CMAKE_CXX_COMPILER_ID MATCHES "AppleClang" AND (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS "7.0"))
			SET(OpenMP_LIBS ${OpenMP_libomp_LIBRARY})
			LIST(APPEND OpenMVS_EXTRA_LIBS ${OpenMP_LIBS})
		endif()
	else()
		MESSAGE("-- Can't find OpenMP. Continuing without it.")
	endif()
endif()

# find_package(hdf5 REQUIRED)
# if(HDF5_FOUND)
#     LIST(APPEND OpenMVS_EXTRA_INCLUDES ${HDF5_INCLUDE_DIRS})
#     LIST(APPEND OpenMVS_EXTRA_LIBS "hdf5::hdf5-shared;hdf5::hdf5_hl-shared;hdf5::hdf5_cpp-shared;hdf5::hdf5_hl_cpp-shared")
#     INCLUDE_DIRECTORIES(${HDF5_INCLUDE_DIRS})
#     ADD_DEFINITIONS(${HDF5_DEFINITIONS})
#     SET(_USE_HDF5 TRUE)
#     MESSAGE(STATUS "HDF5 ${HDF5_VERSION} found (include: ${HDF5_INCLUDE_DIRS} lib: ${HDF5_LIBRARIES})")
# else()
#     MESSAGE(FATAL_ERROR "-- Can't find HDF5. Please specify HDF5 directory using HDF5_DIR variable")
# endif()

# ---------------- HDF5 (manual) ----------------
set(HDF5_ROOT "$ENV{HOME}/.local" CACHE PATH "HDF5 install prefix")

find_path(HDF5_INCLUDE_DIR
    NAMES hdf5.h
    PATHS "${HDF5_ROOT}/include"
    NO_DEFAULT_PATH
)

find_library(HDF5_C_LIBRARY
    NAMES hdf5
    PATHS "${HDF5_ROOT}/lib"
    NO_DEFAULT_PATH
)

find_library(HDF5_CPP_LIBRARY
    NAMES hdf5_cpp
    PATHS "${HDF5_ROOT}/lib"
    NO_DEFAULT_PATH
)

find_library(HDF5_HL_LIBRARY
    NAMES hdf5_hl
    PATHS "${HDF5_ROOT}/lib"
    NO_DEFAULT_PATH
)

find_library(HDF5_HL_CPP_LIBRARY
    NAMES hdf5_hl_cpp
    PATHS "${HDF5_ROOT}/lib"
    NO_DEFAULT_PATH
)

if(NOT HDF5_INCLUDE_DIR)
    message(FATAL_ERROR "HDF5 include dir not found: ${HDF5_ROOT}/include")
endif()

if(NOT HDF5_C_LIBRARY)
    message(FATAL_ERROR "HDF5 C library not found: ${HDF5_ROOT}/lib/libhdf5*")
endif()

if(NOT HDF5_CPP_LIBRARY)
    message(FATAL_ERROR "HDF5 C++ library not found: ${HDF5_ROOT}/lib/libhdf5_cpp*")
endif()

if(NOT HDF5_HL_LIBRARY)
    message(FATAL_ERROR "HDF5 HL library not found: ${HDF5_ROOT}/lib/libhdf5_hl*")
endif()

if(NOT HDF5_HL_CPP_LIBRARY)
    message(FATAL_ERROR "HDF5 HL C++ library not found: ${HDF5_ROOT}/lib/libhdf5_hl_cpp*")
endif()

list(APPEND OpenMVS_EXTRA_INCLUDES ${HDF5_INCLUDE_DIR})
include_directories(${HDF5_INCLUDE_DIR})

list(APPEND OpenMVS_EXTRA_LIBS
    ${HDF5_C_LIBRARY}
    ${HDF5_CPP_LIBRARY}
    ${HDF5_HL_LIBRARY}
    ${HDF5_HL_CPP_LIBRARY}
)

set(_USE_HDF5 TRUE)
message(STATUS "Using manual HDF5")
message(STATUS "HDF5 include: ${HDF5_INCLUDE_DIR}")
message(STATUS "HDF5 libs: ${HDF5_C_LIBRARY};${HDF5_CPP_LIBRARY};${HDF5_HL_LIBRARY};${HDF5_HL_CPP_LIBRARY}")
# ----------------------------------------------


if(OpenMVS_USE_OPENGL)
	FIND_PACKAGE(OpenGL)
	if(OPENGL_FOUND)
		INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_DIR})
		ADD_DEFINITIONS(${OpenGL_DEFINITIONS})
		SET(_USE_OPENGL TRUE)
	else()
		MESSAGE("-- Can't find OpenGL. Continuing without it.")
	endif()
endif()

if(OpenMVS_USE_CUDA)
	INCLUDE(CheckLanguage)
	CHECK_LANGUAGE(CUDA)
	if(CMAKE_CUDA_COMPILER)
		SET(CUDA_FOUND TRUE)
		if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
			SET(CMAKE_CUDA_ARCHITECTURES "native")
		endif()
		ENABLE_LANGUAGE(CUDA)
		# not required, but forced by OpenCV old support for CUDA
		FIND_PACKAGE(CUDA REQUIRED)
		
		find_package(CUDAToolkit REQUIRED)
		# CUDA-11.x can not be compiled using C++14 standard on Windows
		string(REGEX MATCH "^[0-9]+" CUDA_VERSION_MAJOR ${CMAKE_CUDA_COMPILER_VERSION})
		string(REGEX REPLACE "([0-9]+)\\.([0-9]+).*" "\\2" CUDA_VERSION_MINOR ${CMAKE_CUDA_COMPILER_VERSION})
		if(${CUDA_VERSION_MAJOR} GREATER 10)
			SET(CMAKE_CUDA_STANDARD 17)
		endif()
		EXECUTE_PROCESS(COMMAND "${CMAKE_CUDA_COMPILER}" --list-gpu-arch 
				OUTPUT_VARIABLE LIST_GPU_ARCH 
				ERROR_QUIET)
		if(NOT LIST_GPU_ARCH AND OpenMVS_MAX_CUDA_COMPATIBILITY)
			message(WARNING "Cannot compile for max CUDA compatibility, nvcc does not support --list-gpu-arch")
			SET(OpenMVS_MAX_CUDA_COMPATIBILITY OFF)
		endif()
		if(NOT OpenMVS_MAX_CUDA_COMPATIBILITY)
			if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
				SET(CMAKE_CUDA_ARCHITECTURES 89)
			endif()
		else()
			# Build for maximum compatibility
			# https://arnon.dk/matching-sm-architectures-arch-and-gencode-for-various-nvidia-cards/

			# Extract list of arch and gencodes
			STRING(REPLACE "\r" "" LIST_GPU_ARCH ${LIST_GPU_ARCH})
			STRING(REPLACE "\n" ";" LIST_GPU_ARCH ${LIST_GPU_ARCH})

			EXECUTE_PROCESS(COMMAND "${CMAKE_CUDA_COMPILER}" --list-gpu-code 
				OUTPUT_VARIABLE LIST_GPU_CODE 
				ERROR_QUIET)
			STRING(REPLACE "\r" "" LIST_GPU_CODE ${LIST_GPU_CODE})
			STRING(REPLACE "\n" ";" LIST_GPU_CODE ${LIST_GPU_CODE})

			LIST(GET LIST_GPU_CODE 0 TARGET_GPU_CODE)
			SET(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -arch=${TARGET_GPU_CODE}")

			SET(IDX 0)
			foreach(GPU_ARCH ${LIST_GPU_ARCH})
				LIST(GET LIST_GPU_CODE ${IDX} GPU_CODE)
				SET(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -gencode=arch=${GPU_ARCH},code=${GPU_CODE}")
				MATH(EXPR IDX "${IDX}+1")
			endforeach()
			MESSAGE("-- Set CUDA flags: " ${CMAKE_CUDA_FLAGS})
		endif()
		SET(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --expt-relaxed-constexpr")
		INCLUDE_DIRECTORIES(${CUDA_INCLUDE_DIRS})
		SET(_USE_CUDA TRUE)
		MESSAGE("-- Found CUDA: " ${CMAKE_CUDA_COMPILER_VERSION} " with GPU arch: " ${CMAKE_CUDA_ARCHITECTURES})
	else()
		SET(CUDA_CUDA_LIBRARY "")
		MESSAGE("-- Can't find CUDA. Continuing without it.")
	endif()
else()
	SET(CUDA_CUDA_LIBRARY "")
endif()

if(OpenMVS_USE_BREAKPAD)
	FIND_PACKAGE(BREAKPAD)
	if(BREAKPAD_FOUND)
		INCLUDE_DIRECTORIES(${BREAKPAD_INCLUDE_DIRS})
		ADD_DEFINITIONS(${BREAKPAD_DEFINITIONS})
		SET(_USE_BREAKPAD TRUE)
		LIST(APPEND OpenMVS_EXTRA_LIBS ${BREAKPAD_LIBS})
	else()
		MESSAGE("-- Can't find BreakPad. Continuing without it.")
	endif()
endif()

SET(Boost_EXTRA_COMPONENTS "")
if(OpenMVS_USE_PYTHON)
	FIND_PACKAGE(Python3 COMPONENTS Interpreter Development REQUIRED)
	if(Python3_FOUND)
		INCLUDE_DIRECTORIES(${Python3_INCLUDE_DIRS})
		LIST(APPEND OpenMVS_EXTRA_INCLUDES ${Python3_INCLUDE_DIRS})
		LIST(APPEND OpenMVS_EXTRA_LIBS ${Python3_LIBRARIES})
		LIST(APPEND Boost_EXTRA_COMPONENTS python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR})
		MESSAGE(STATUS "Python ${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR} found (include: ${Python3_INCLUDE_DIRS})")
	else()
		MESSAGE("-- Can't find Python. Continuing without it.")
	endif()
endif()

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 OLD)
endif()

set(Boost_NO_BOOST_CMAKE ON)

FIND_PACKAGE(Boost REQUIRED
    COMPONENTS iostreams program_options serialization
    OPTIONAL_COMPONENTS system ${Boost_EXTRA_COMPONENTS}
)

if(Boost_FOUND)
    LIST(APPEND OpenMVS_EXTRA_INCLUDES ${Boost_INCLUDE_DIRS})
    INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
    ADD_DEFINITIONS(${Boost_DEFINITIONS})
    LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})

    # 优先使用 imported targets
    if(TARGET Boost::iostreams)
        LIST(APPEND OpenMVS_EXTRA_LIBS
            Boost::iostreams
            Boost::program_options
            Boost::serialization
        )

        # Boost.System 在 1.86 里通常是 header-only；有 target 才链接
        if(TARGET Boost::system)
            LIST(APPEND OpenMVS_EXTRA_LIBS Boost::system)
        endif()
    else()
        LIST(APPEND OpenMVS_EXTRA_LIBS ${Boost_LIBRARIES})
    endif()

    # Python 组件
    if(OpenMVS_USE_PYTHON)
        set(_BOOST_PYTHON_FOUND FALSE)

        foreach(_py_tgt
            Boost::python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}
            Boost::python${Python3_VERSION_MAJOR}
            Boost::python
        )
            if(TARGET ${_py_tgt})
                LIST(APPEND OpenMVS_EXTRA_LIBS ${_py_tgt})
                set(_BOOST_PYTHON_FOUND TRUE)
                break()
            endif()
        endforeach()

        if(_BOOST_PYTHON_FOUND)
            SET(_USE_BOOST_PYTHON TRUE)
        else()
            MESSAGE(WARNING "Boost.Python target not found; Python bindings will be disabled")
        endif()
    endif()

    # zstd 兜底
    find_library(ZSTD_LIBRARY zstd
        PATHS
            $ENV{HOME}/.local/lib
            /usr/lib64
            /usr/lib/x86_64-linux-gnu
    )
    if(ZSTD_LIBRARY)
        LIST(APPEND OpenMVS_EXTRA_LIBS ${ZSTD_LIBRARY})
    endif()

    SET(_USE_BOOST TRUE)
endif()

FIND_PACKAGE(Eigen3 3.4 REQUIRED)
if(EIGEN3_FOUND)
	LIST(APPEND OpenMVS_EXTRA_INCLUDES ${EIGEN3_INCLUDE_DIR})
	INCLUDE_DIRECTORIES(${EIGEN3_INCLUDE_DIR})
	ADD_DEFINITIONS(${EIGEN3_DEFINITIONS})
	SET(_USE_EIGEN TRUE)
	MESSAGE(STATUS "Eigen ${EIGEN3_VERSION} found (include: ${EIGEN3_INCLUDE_DIR})")
endif()

FIND_PACKAGE(OpenCV REQUIRED)
if(OpenCV_FOUND)
	LIST(APPEND OpenMVS_EXTRA_INCLUDES ${OpenCV_INCLUDE_DIRS})
	INCLUDE_DIRECTORIES(${OpenCV_INCLUDE_DIRS})
	ADD_DEFINITIONS(${OpenCV_DEFINITIONS})
	SET(_USE_OPENCV TRUE)
	MESSAGE(STATUS "OpenCV ${OpenCV_VERSION} found (include: ${OpenCV_INCLUDE_DIRS})")
else()
	MESSAGE("-- Can't find OpenCV. Please specify OpenCV directory using OpenCV_DIR variable")
endif()

set(MKL_COMPONENTS
    intel_lp64
    intel_thread
    core
)
SET(FAISS_EXTRA_LIBS "")
set(MKL_INTERFACE_FULL "intel_lp64")
find_package(MKL REQUIRED COMPONENTS ${MKL_COMPONENTS})
if(MKL_FOUND)
	message(STATUS "Using Intel MKL for BLAS/LAPACK")
	MESSAGE(STATUS "Intel MKL ${MKL_VERSION} found (libs: ${MKL_LIBRARIES})")
	if(NOT MKL_LIBRARIES)
		if(MSVC)
			set(MKL_LIBRARIES
				"${MKL_ROOT}/lib/mkl_intel_lp64_dll.lib"
				"${MKL_ROOT}/lib/mkl_intel_thread_dll.lib"
				"${MKL_ROOT}/lib/mkl_core_dll.lib"
				"${MKL_ROOT}/../../compiler/latest/lib/libiomp5md.lib"
			)
			message(STATUS "Manually set MKL libraries: ${MKL_LIBRARIES}")
		else()
			set(MKL_LIBRARIES
				"${MKL_ROOT}/lib/libmkl_intel_lp64.a"
				"${MKL_ROOT}/lib/libmkl_intel_thread.a"
				"${MKL_ROOT}/lib/libmkl_core.a"
				"${MKL_ROOT}/../../compiler/latest/lib/libiomp5.a"
			)
			message(STATUS "Manually set MKL libraries: ${MKL_LIBRARIES}")
		endif()
    endif()
	list(APPEND OpenMVS_EXTRA_LIBS ${MKL_LIBRARIES})
	list(APPEND FAISS_EXTRA_LIBS ${MKL_LIBRARIES})
else()
	MESSAGE(STATUS "-- Can't find MKL. Please specify MKL directory using MKL_DIR variable")
endif()

find_package(FAISS REQUIRED)

if(TARGET faiss)
    get_target_property(FAISS_INCLUDE_DIRS faiss INTERFACE_INCLUDE_DIRECTORIES)
    list(APPEND OpenMVS_EXTRA_INCLUDES ${FAISS_INCLUDE_DIRS})
    list(APPEND OpenMVS_EXTRA_LIBS faiss)
    list(APPEND FAISS_EXTRA_LIBS faiss)
    set(_USE_FAISS TRUE)

    if(OpenMVS_USE_CUDA)
        if(TARGET faiss_gpu)
            get_target_property(FAISS_GPU_INCLUDE_DIRS faiss_gpu INTERFACE_INCLUDE_DIRECTORIES)
            list(APPEND OpenMVS_EXTRA_INCLUDES ${FAISS_GPU_INCLUDE_DIRS})
            list(APPEND OpenMVS_EXTRA_LIBS faiss_gpu)
            set(_USE_FAISS_GPU TRUE)
            message(STATUS "FAISS-GPU enabled with separate faiss_gpu target")
        else()
            message(WARNING "No separate faiss_gpu target exported by FAISS; using faiss only")
        endif()
    endif()
else()
    message(FATAL_ERROR "FAISS target not found")
endif()

LIST(REMOVE_DUPLICATES OpenMVS_EXTRA_INCLUDES)
LIST(REMOVE_DUPLICATES OpenMVS_EXTRA_LIBS)

# Set defines
if(OpenMVS_USE_FAST_FLOAT2INT)
	SET(_USE_FAST_FLOAT2INT TRUE)
endif()
if(OpenMVS_USE_FAST_INVSQRT)
	SET(_USE_FAST_INVSQRT TRUE)
endif()
if(OpenMVS_USE_FAST_CBRT)
	SET(_USE_FAST_CBRT TRUE)
endif()
if(OpenMVS_USE_SSE)
	SET(_USE_SSE TRUE)
endif()
if(OpenMVS_USE_CERES)
	SET(_USE_CERES TRUE)
endif()

INCLUDE_DIRECTORIES("${OpenMVS_SOURCE_DIR}")
INCLUDE_DIRECTORIES("${CMAKE_BINARY_DIR}")

# Add modules
ADD_SUBDIRECTORY(libs)
if(OpenMVS_BUILD_TOOLS)
	ADD_SUBDIRECTORY(apps)
endif()
ADD_SUBDIRECTORY(docs)

if (OpenMVS_ENABLE_TESTS)
	# enable testing functionality
	ENABLE_TESTING()
	# define tests
	ADD_TEST(NAME UnitTests COMMAND $<TARGET_FILE:Tests> "0")
	ADD_TEST(NAME PipelineTest COMMAND $<TARGET_FILE:Tests> "1")
endif()

# Export the package for use from the build-tree
# (this registers the build-tree with a global CMake-registry)
EXPORT(PACKAGE OpenMVS)

# Install the export set for use with the install-tree
INSTALL(EXPORT OpenMVSTargets
	NAMESPACE OpenMVS::
	DESTINATION "${INSTALL_CMAKE_DIR}")

# Install configuration file
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/build/Templates/ConfigLocal.h.in" "${CMAKE_BINARY_DIR}/ConfigLocal.h")
INSTALL(FILES "${CMAKE_BINARY_DIR}/ConfigLocal.h" DESTINATION "${INSTALL_INCLUDE_DIR}")

# Create the OpenMVSConfig.cmake and OpenMVSConfigVersion files
INCLUDE(CMakePackageConfigHelpers)
write_basic_package_version_file("${PROJECT_BINARY_DIR}/OpenMVSConfigVersion.cmake"
	VERSION ${OpenMVS_VERSION}
	COMPATIBILITY AnyNewerVersion)
SET(INSTALL_INCLUDE_DIR_IN ${INSTALL_INCLUDE_DIR_PREFIX} ${OpenMVS_EXTRA_INCLUDES})
SET(INSTALL_CMAKE_DIR_IN ${INSTALL_CMAKE_DIR_PREFIX})
configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/build/Templates/OpenMVSConfig.cmake.in"
	"${PROJECT_BINARY_DIR}/OpenMVSConfig.cmake"
	INSTALL_DESTINATION ${PROJECT_BINARY_DIR}
	NO_SET_AND_CHECK_MACRO)
# Install the OpenMVSConfig.cmake and OpenMVSConfigVersion.cmake
INSTALL(FILES
	"${PROJECT_BINARY_DIR}/OpenMVSConfig.cmake"
	"${PROJECT_BINARY_DIR}/OpenMVSConfigVersion.cmake"
	DESTINATION "${INSTALL_CMAKE_DIR}")

# uninstall target
CONFIGURE_FILE(
	"${CMAKE_CURRENT_SOURCE_DIR}/build/Templates/cmake_uninstall.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
	IMMEDIATE @ONLY)

ADD_CUSTOM_TARGET(uninstall
	COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
