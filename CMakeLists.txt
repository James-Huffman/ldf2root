cmake_minimum_required(VERSION 3.13)
project(ldf2root C CXX)
include(CheckCXXCompilerFlag)

if(NOT CMAKE_CXX_STANDARD)
	set(CMAKE_CXX_STANDARD 20)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

check_cxx_compiler_flag("-Wc++20-extensions" COMPILER_SUPPORTS_WCXX20_EXTENSIONS)
check_cxx_compiler_flag("-Wc++20-attribute-extensions" COMPILER_SUPPORTS_WCXX20_ATTRIBUTE_EXTENSIONS)
if(COMPILER_SUPPORTS_WCXX20_EXTENSIONS)
	if(NOT CMAKE_CXX_STANDARD STREQUAL "20")
		add_compile_options(-Wc++20-extensions)
	else()
		add_compile_options(-Wno-c++20-extensions)
	endif()
endif()
if(COMPILER_SUPPORTS_WCXX20_ATTRIBUTE_EXTENSIONS)
	if(NOT CMAKE_CXX_STANDARD STREQUAL "20")
		add_compile_options(-Wc++20-attribute-extensions)
	else()
		add_compile_options(-Wno-c++20-attribute-extensions)
	endif()
endif()

#Use rpath on Mac OS
set(CMAKE_MACOSX_RPATH TRUE)

if (CMAKE_COMPILER_IS_GNUCXX)
	#Make compiler messages nice with colored tags.
	if (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER "4.9")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=auto")
	endif ()

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -fPIC")
	
	# Suppress false positive array-bounds warnings in GCC 13+ with fmt library
	if (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL "13.0")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-array-bounds")
	endif ()
endif ()

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	message(STATUS "Install Prefix not specified.")
	file(MAKE_DIRECTORY install)
	get_filename_component(INSTALL_DIR ${CMAKE_SOURCE_DIR}/install REALPATH)
	set(CMAKE_INSTALL_PREFIX ${INSTALL_DIR} CACHE PATH "Install Prefix" FORCE)
endif ()
message(STATUS "Installing to ${CMAKE_INSTALL_PREFIX}")

#Add additional CXX flags if we use the Debug option
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build (Release, Debug, RelWithDebInfo, MinSizeRel)" FORCE)
	message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo" ) 
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
	message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
endif()

include(CMakeDependentOption)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Generates compile_commands.json, needed for some IDEs among other things" FORCE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/Cmake_modules")


#Find ROOT if USE_ROOT was set.
find_package(ROOT REQUIRED)
if (NOT ROOT_FOUND)
	message(STATUS "ROOT not found, trying to find it in the environment.")
	set(ROOTSYS $ENV{ROOTSYS} CACHE STRING "ROOT directory.")
	if (NOT EXISTS ${ROOTSYS})
		message(FATAL_ERROR "ROOTSYS environment variable is not set or does not point to a valid ROOT installation.")
	endif ()
endif ()
include(${ROOT_USE_FILE})
set(ROOTSYS $ENV{ROOTSYS} CACHE STRING "ROOT directory.")
message(STATUS "FOUND ROOT LOCATED HERE: ${ROOTSYS}")
find_program(ROOT_CONFIG_EXECUTABLE root-config PATHS ${ROOTSYS}/bin)
find_program(ROOTCINT_EXECUTABLE rootcint PATHS $ENV{ROOTSYS}/bin)
find_program(GENREFLEX_EXECUTABLE genreflex PATHS $ENV{ROOTSYS}/bin)
if (ROOT_CONFIG_EXECUTABLE)
	execute_process(
	COMMAND ${ROOT_CONFIG_EXECUTABLE} --prefix
	OUTPUT_VARIABLE ROOTSYS
	OUTPUT_STRIP_TRAILING_WHITESPACE)
	message(STATUS "ROOTSYS: ${ROOTSYS}")

	execute_process(
	COMMAND ${ROOT_CONFIG_EXECUTABLE} --version
	OUTPUT_VARIABLE ROOT_VERSION
	OUTPUT_STRIP_TRAILING_WHITESPACE)
	message(STATUS "ROOT Version: ${ROOT_VERSION}")

	execute_process(
	COMMAND ${ROOT_CONFIG_EXECUTABLE} --incdir
	OUTPUT_VARIABLE ROOT_INCLUDE_DIR
	OUTPUT_STRIP_TRAILING_WHITESPACE)
	message(STATUS "ROOT Include Dir: ${ROOT_INCLUDE_DIR}")

	execute_process(
	COMMAND ${ROOT_CONFIG_EXECUTABLE} --libs
	OUTPUT_VARIABLE ROOT_LIBRARIES
	OUTPUT_STRIP_TRAILING_WHITESPACE)
	message(STATUS "ROOT Libraries: ${ROOT_LIBRARIES}")

	execute_process(
	COMMAND ${root_config_executable} --glibs
	OUTPUT_VARIABLE ROOT_GUI_LIBRARIES
	OUTPUT_STRIP_TRAILING_WHITESPACE)
	message(STATUS "ROOT Gui Libraries: ${ROOT_GUI_LIBRARIES}")

	execute_process(
	COMMAND ${root_config_executable} --evelibs
	OUTPUT_VARIABLE ROOT_EVE_GUI_LIBRARIES
	OUTPUT_STRIP_TRAILING_WHITESPACE)
	message(STATUS "ROOT Eve Gui Libraries: ${ROOT_EVE_GUI_LIBRARIES}")

	set(ROOT_LIBRARY_DIR ${ROOTSYS}/lib)

	execute_process(
	COMMAND ${ROOT_CONFIG_EXECUTABLE} --has-minuit2
	OUTPUT_VARIABLE ROOT_HAS_MINUIT2
	OUTPUT_STRIP_TRAILING_WHITESPACE)

	execute_process(
	COMMAND ${ROOT_CONFIG_EXECUTABLE} --has-cxx17
	OUTPUT_VARIABLE ROOT_HAS_CXX17)

	execute_process(
	COMMAND ${ROOT_CONFIG_EXECUTABLE} --has-cxx14
	OUTPUT_VARIABLE ROOT_HAS_CXX14)

	execute_process(
	COMMAND ${ROOT_CONFIG_EXECUTABLE} --has-cxx11
	OUTPUT_VARIABLE ROOT_HAS_CXX11)

endif ()

if (${ROOT_VERSION} VERSION_LESS "6.10")
	message(FATAL_ERROR "Minimum ROOT version is 6.10.00.")
endif (${ROOT_VERSION} VERSION_LESS "6.10")
mark_as_advanced( GENREFLEX_EXECUTABLE ROOTCINT_EXECUTABLE ROOT_CONFIG_EXECUTABLE)

#----------------------------------------------------------------------------s
include(ExternalProject)

set(EXTERNAL_DEPS)
set(EXTERNAL_CORE_DEPS)
set(EXTERNAL_TRANSLATOR_DEPS)
set(EXTERNAL_PROCESSOR_DEPS)
set(EXTERNAL_LOG_DEPS)

set(_LOG_OPTIONS 
	LOG_DOWNLOAD TRUE
	LOG_UPDATE TRUE
	LOG_TEST FALSE
	GIT_PROGRESS TRUE
	LOG_CONFIGURE TRUE
	LOG_BUILD TRUE
	LOG_INSTALL TRUE
	LOG_PATCH TRUE
	LOG_OUTPUT_ON_FAILURE TRUE
)

# option(USE_SYSTEM_FMT "Use the system fmt library, if not then we will download and use that one" OFF)
# if( USE_SYSTEM_FMT )
	find_package(fmt REQUIRED)
	set(_FMT_NAME fmt)
# else()
# 	set(FMT_EXTERNAL_INSTALL_LOCATION ${CMAKE_INSTALL_PREFIX}/external)
# 	set(FMT_VERSION 8.1.1 CACHE STRING "Version of fmt you want to get from git" )
# 	mark_as_advanced(FMT_VERSION)
# 	set(_FMT_NAME fmtProject)
# 	ExternalProject_Add(fmtProject
# 		GIT_REPOSITORY https://github.com/fmtlib/fmt
# 		GIT_TAG ${FMT_VERSION}
# 		CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${FMT_EXTERNAL_INSTALL_LOCATION} -DCMAKE_VERBOSE_MAKEFILE=OFF -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
# 		BUILD_COMMAND make -s -j 10
# 		INSTALL_COMMAND make -s install
# 		${_LOG_OPTIONS}
# 	)
# 	include_directories(${FMT_EXTERNAL_INSTALL_LOCATION}/include)
# 	link_directories(${FMT_EXTERNAL_INSTALL_LOCATION}/lib)
# 	list(APPEND EXTERNAL_DEPS fmtProject)
# 	list(APPEND EXTERNAL_LOG_DEPS fmtProject)
# endif()

# option(USE_SYSTEM_SPDLOG "Use either spdlog or grab it from github" OFF)
# if( USE_SYSTEM_SPDLOG )
    find_package(spdlog REQUIRED)
# else()
# 	set(SPDLOG_EXTERNAL_INSTALL_LOCATION ${CMAKE_INSTALL_PREFIX}/external)
# 	set(SPDLOG_VERSION v1.9.2 CACHE STRING "Version of spdlog you want to get from git" )
# 	mark_as_advanced(SPDLOG_VERSION)
# 	ExternalProject_Add(spdlogProject
# 		GIT_REPOSITORY https://github.com/gabime/spdlog
# 		GIT_TAG ${SPDLOG_VERSION}
# 		CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${SPDLOG_EXTERNAL_INSTALL_LOCATION} -DCMAKE_VERBOSE_MAKEFILE=OFF -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
# 		BUILD_COMMAND make -s -j 10
# 		INSTALL_COMMAND make -s install
# 		${_LOG_OPTIONS}
# 		DEPENDS ${_FMT_NAME}
# 	)
# 	include_directories(${SPDLOG_EXTERNAL_INSTALL_LOCATION}/include)
# 	link_directories(${SPDLOG_EXTERNAL_INSTALL_LOCATION}/lib)
# 	list(APPEND EXTERNAL_DEPS spdlogProject)
# 	list(APPEND EXTERNAL_LOG_DEPS spdlogProject)
# endif()


add_subdirectory(core)

add_executable(ldf2root ldf2root.cpp)

# Link libraries
# if( EXTERNAL_DEPS )
# 	add_dependencies(ldf2root ${EXTERNAL_DEPS})
# endif()
target_link_libraries(ldf2root PRIVATE ${ROOT_LIBRARIES} DDASRoot DDASRootLegacy ldf2rootCore ROOT::RIO)
target_include_directories(ldf2root PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# if( USE_SYSTEM_FMT )
	target_link_libraries(ldf2root PRIVATE fmt::fmt)
# endif()
# if( USE_SYSTEM_SPDLOG )
	target_link_libraries(ldf2root PRIVATE spdlog::spdlog_header_only)
# endif()

# Install target (optional)
install(TARGETS ldf2root RUNTIME DESTINATION bin)
#Configure and install the module file
configure_file("modulefiles/ldf2root" modulefiles/ldf2root @ONLY)
install(DIRECTORY ${CMAKE_BINARY_DIR}/modulefiles  DESTINATION ${CMAKE_INSTALL_PREFIX})