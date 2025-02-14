project (widelands)

cmake_minimum_required (VERSION 3.14)

option(USE_SDL12_COMPAT "Replace SDL1.2 library with sdl12-compat" ON)

set(USE_SDL12_SHARED_DEFAULT ON)
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  set(USE_SDL12_SHARED_DEFAULT OFF)
endif()

option(USE_SDL12_SHARED "Replace SDL1.2 with shared sdl12-compat" ${USE_SDL12_SHARED_DEFAULT})

# Used as versioning, if WL_VERSION_STANDARD is definded.
if (WL_VERSION_STANDARD)
	# Set the values below for standard versioning.
	# As long as WL_VERSION_MAJOR is 0, the naming will be in form of
	# "Build${WL_VERSION_MINOR}" + ".${WL_VERSION_PATCH}" if WL_VERSION_PATCH is
	# defined.
	set (WL_VERSION_MAJOR 0)
	set (WL_VERSION_MINOR 15)

	# Just define in case that we release patches for official builds
	#set (WL_VERSION_PATCH 90)

	# An addition to the version name (e.g. RC1, DEV, ...)
	set (WL_VERSION_ADDITION DEV)
endif (WL_VERSION_STANDARD)

# Define the directory structure for installation - will be hardcoded in WL bin
# If nothing is given on command line, set the standard values depending on platform.

if (WIN32 OR APPLE OR WL_PORTABLE)
  if (NOT WL_INSTALL_PREFIX OR WL_INSTALL_PREFIX STREQUAL "")
    set (WL_INSTALL_PREFIX ".")
  endif (NOT WL_INSTALL_PREFIX OR WL_INSTALL_PREFIX STREQUAL "")
  
  if (NOT WL_INSTALL_DATADIR OR WL_INSTALL_DATADIR STREQUAL "")
    set (WL_INSTALL_DATADIR ".")
  endif (NOT WL_INSTALL_DATADIR OR WL_INSTALL_DATADIR STREQUAL "")

  if (NOT WL_INSTALL_LOCALEDIR OR WL_INSTALL_LOCALEDIR STREQUAL "")
    set (WL_INSTALL_LOCALEDIR "locale")
  endif (NOT WL_INSTALL_LOCALEDIR OR WL_INSTALL_LOCALEDIR STREQUAL "")

  if (NOT WL_INSTALL_BINDIR OR WL_INSTALL_BINDIR STREQUAL "")
    set (WL_INSTALL_BINDIR ".")
  endif (NOT WL_INSTALL_BINDIR OR WL_INSTALL_BINDIR STREQUAL "")
elseif (WIN32 OR APPLE OR WL_PORTABLE)
  if (NOT WL_INSTALL_PREFIX OR WL_INSTALL_PREFIX STREQUAL "")
    set (WL_INSTALL_PREFIX "/usr/local")
  endif (NOT WL_INSTALL_PREFIX OR WL_INSTALL_PREFIX STREQUAL "")
  
  if (NOT WL_INSTALL_DATADIR OR WL_INSTALL_DATADIR STREQUAL "")
    set (WL_INSTALL_DATADIR "share/games/widelands")
  endif (NOT WL_INSTALL_DATADIR OR WL_INSTALL_DATADIR STREQUAL "")

  if (NOT WL_INSTALL_LOCALEDIR OR WL_INSTALL_LOCALEDIR STREQUAL "")
    set (WL_INSTALL_LOCALEDIR "${WL_INSTALL_PREFIX}/${WL_INSTALL_DATADIR}/locale")
  endif (NOT WL_INSTALL_LOCALEDIR OR WL_INSTALL_LOCALEDIR STREQUAL "")

  if (NOT WL_INSTALL_BINDIR OR WL_INSTALL_BINDIR STREQUAL "")
    set (WL_INSTALL_BINDIR "games")
  endif (NOT WL_INSTALL_BINDIR OR WL_INSTALL_BINDIR STREQUAL "")
endif (WIN32 OR APPLE OR WL_PORTABLE)

# General install prefix for config.h
if (NOT WL_INSTALL_PREFIX OR WL_INSTALL_PREFIX STREQUAL "")
	set (WL_INSTALL_PREFIX /usr/local)
endif (NOT WL_INSTALL_PREFIX OR WL_INSTALL_PREFIX STREQUAL "")

# Widelands binary directory
if (NOT WL_INSTALL_BINDIR OR WL_INSTALL_BINDIR STREQUAL "")
	set (WL_INSTALL_BINDIR games)
endif (NOT WL_INSTALL_BINDIR OR WL_INSTALL_BINDIR STREQUAL "")

# Widelands data directory
if (NOT WL_INSTALL_DATADIR OR WL_INSTALL_DATADIR STREQUAL "")
	set (WL_INSTALL_DATADIR share/games/widelands)
endif (NOT WL_INSTALL_DATADIR OR WL_INSTALL_DATADIR STREQUAL "")

# Widelands locale directory
if (NOT WL_INSTALL_LOCALEDIR OR WL_INSTALL_LOCALEDIR STREQUAL "")
	set (WL_INSTALL_LOCALEDIR ${WL_INSTALL_PREFIX}/${WL_INSTALL_DATADIR}/locale)
endif (NOT WL_INSTALL_LOCALEDIR OR WL_INSTALL_LOCALEDIR STREQUAL "")

set (WL_SOURCE_CHECKER ${CMAKE_CURRENT_SOURCE_DIR}/cmake/codecheck/CodeCheck.py)

set (WLBUILD_INSTALL_PREFIX ${WL_INSTALL_PREFIX})
set (WLBUILD_INSTALL_DATADIR ${WL_INSTALL_DATADIR})
set (WLBUILD_INSTALL_LOCALEDIR "${WL_INSTALL_DATADIR}/locale")
set (WLBUILD_INSTALL_BINDIR ${WL_INSTALL_BINDIR})

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR "Build-directory and source-directory are the same!")
endif (CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)

if (CMAKE_INSTALL_PREFIX STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR "Build-directory and install-directory are the same!")
endif (CMAKE_INSTALL_PREFIX STREQUAL CMAKE_BINARY_DIR)

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules)

if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
  find_path(FILE_WL_RELEASE "WL_RELEASE" ${CMAKE_CURRENT_SOURCE_DIR})
  if (${FILE_WL_RELEASE} STREQUAL "FILE_WL_RELEASE-NOTFOUND")
    set (CMAKE_BUILD_TYPE Debug)
  else (${FILE_WL_RELEASE} STREQUAL "FILE_WL_RELEASE-NOTFOUND")
    set (CMAKE_BUILD_TYPE Release)
  endif (${FILE_WL_RELEASE} STREQUAL "FILE_WL_RELEASE-NOTFOUND")
endif (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")

set (CMAKE_CONFIGURATION_TYPES ${CMAKE_BUILD_TYPE})

set (BUILD_SHARED_LIBS OFF)
set (Boost_USE_STATIC_LIBS   ON)
set (Boost_USE_MULTITHREADED ON)
find_package(Boost 1.35.0 REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

set (PARAMETER_COMPILERFLAG_OLDSTYLECAST_EXTENDED "-Wold-style-cast")
set (PARAMETER_COMPILERFLAG_OLDSTYLECAST "-Wold-style-cast")
set (PARAMETER_COMPILERFLAG_GENERICWARNINGS "-Wno-attributes -Wall")
IF (CMAKE_BUILD_TYPE STREQUAL "Debug")
  include(CheckCXXCompilerFlag) #this include should be safe
  CHECK_CXX_COMPILER_FLAG(${PARAMETER_COMPILERFLAG_OLDSTYLECAST_EXTENDED} Compiler_old-style-cast_supported)
  IF (Compiler_old-style-cast_supported)
    EXEC_PROGRAM(${CMAKE_CXX_COMPILER}
      ARGS --version
      OUTPUT_VARIABLE WLBUILD_COMPILERVERSION
    )
    STRING(REGEX REPLACE ".*([0-9])\\.([0-9])\\.([0-9]).*" "\\1.\\2.\\3" WLBUILD_COMPILERVERSION_REP ${WLBUILD_COMPILERVERSION})
    IF (WLBUILD_COMPILERVERSION_REP STREQUAL "4.3.2" OR WLBUILD_COMPILERVERSION_REP STREQUAL "4.3.1")
      message("===== ATTENTION ===================================================")
      message("Your compiler has been identified as GCC 4.3.1 or GCC 4.3.2")
      message(" ")
      message("There is a bug regarding this version, see http://gcc.gnu.org/bugzilla/show_bug.cgi?id=38000")
      message("and https://bugs.launchpad.net/widelands/+bug/549479 for details.")
      message("Build will disable -isystem usage, expect lots of warnings from Boost header files.")
      message("Widelands should still compile and link correctly.")
      message("If the compiler identification is correct, please consider updating your gcc.")
      message("This warning message will disappear at GCC 4.3.3 and higher.")
      message("If you feel this is wrong, please submit a bug with widelands (not with GCC!)")
      message("and add the following information how cmake sees your C++ compiler:")
      message(" ")
      message("--------------------------------")
      message("${WLBUILD_COMPILERVERSION}")
      message("--------------------------------")
      message(" ")
      message("Thank you for taking your time to read this.")
      message("===================================================================")
      set (WL_COMPILERFLAG_OLDSTYLECAST " ${PARAMETER_COMPILERFLAG_OLDSTYLECAST}") #the space is on purpose!
    ELSE (WLBUILD_COMPILERVERSION_REP STREQUAL "4.3.2" OR WLBUILD_COMPILERVERSION_REP STREQUAL "4.3.1")
      set (WL_COMPILERFLAG_OLDSTYLECAST " ${PARAMETER_COMPILERFLAG_OLDSTYLECAST_EXTENDED}") #the space is on purpose!
    ENDIF (WLBUILD_COMPILERVERSION_REP STREQUAL "4.3.2" OR WLBUILD_COMPILERVERSION_REP STREQUAL "4.3.1")
  ENDIF (Compiler_old-style-cast_supported)

  CHECK_CXX_COMPILER_FLAG(${PARAMETER_COMPILERFLAG_GENERICWARNINGS} Compiler_generic_warnings_supported)
  IF (Compiler_generic_warnings_supported)
    set (WL_COMPILERFLAG_GENERICWARNINGS " ${PARAMETER_COMPILERFLAG_GENERICWARNINGS}") #the space is on purpose!
  ENDIF (Compiler_generic_warnings_supported)
ENDIF (CMAKE_BUILD_TYPE STREQUAL "Debug")

# CMAKE only defines "-g", but we need -DDEBUG also, and we need -DNOPARACHUTE (for SDL) in Debug
set (CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG -DNOPARACHUTE${WL_COMPILERFLAG_OLDSTYLECAST}${WL_COMPILERFLAG_GENERICWARNINGS}" CACHE STRING "Set by widelands CMakeLists.txt" FORCE)

# CMAKE defines "-O3 -DNDEBUG", but we better say -O2 (see gcc manual)
set (CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "Set by widelands CMakeLists.txt" FORCE)

#If building with MSVC, then check for 3rdparty libs
if (DEFINED MSVC)
  set (WL_3RDPARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty" CACHE STRING "Widelands 3rdparty dir")
  set (CMAKE_INCLUDE_PATH "${WL_3RDPARTY_DIR}/include;${WL_3RDPARTY_DIR}/include/sdl")
  set (CMAKE_LIBRARY_PATH "${WL_3RDPARTY_DIR}/lib")

  add_definitions( -Dand=&& -Dor=|| -Dnot=! -Dxor=^ )
  add_definitions( -Dstrcasecmp=_stricmp )
  add_definitions( -Dstrncasecmp=_strnicmp )
  add_definitions( -Dsnprintf=_snprintf )
  add_definitions( -Drint=floor )
  add_definitions( -Dstrtoll=_strtoi64 )
  add_definitions( -Dstrtoull=_strtoui64 )
  add_definitions( -DNOMINMAX -D_CRT_SECURE_NO_WARNINGS )
  add_definitions( -Dssize_t=size_t )
  add_definitions( -Dmkdir=_mkdir )


  find_library(INTL_LIBRARY NAMES intl libintl)

  # CMAKE only defines "-g", but we need -DDEBUG also, and we need -DNOPARACHUTE (for SDL) in Debug
  set (CMAKE_CXX_FLAGS_DEBUG " -DDEBUG -DNOPARACHUTE /MDd /Zi /D_DEBUG${WL_COMPILERFLAG_OLDSTYLECAST}${WL_COMPILERFLAG_GENERICWARNINGS}" CACHE STRING "Set by widelands CMakeLists.txt" FORCE)

  # CMAKE defines "-O3 -DNDEBUG", but we better say -O2 (see gcc manual)
  set (CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG /MD" CACHE STRING "Set by widelands CMakeLists.txt" FORCE)

  set (CMAKE_EXE_LINKER_FLAGS_DEBUG "/debug /INCREMENTAL:YES /NODEFAULTLIB:msvcrt.lib" CACHE STRING "Set by widelands CMakeLists.txt" FORCE)

else (DEFINED MSVC)
  set (HAVE_VARARRAY TRUE CACHE BOOL "Use variable length arrays to improve performance. Requires gcc." )
endif (DEFINED MSVC)

IF (CMAKE_BUILD_TYPE STREQUAL "Debug")
  message (STATUS "Using compiler flags: ${CMAKE_CXX_FLAGS_DEBUG}")
ELSE (CMAKE_BUILD_TYPE STREQUAL "Debug")
  message (STATUS "Using compiler flags: ${CMAKE_CXX_FLAGS_RELEASE}")
ENDIF (CMAKE_BUILD_TYPE STREQUAL "Debug")

if (DEFINED WL_3RDPARTY_DIR)
  set (CMAKE_INCLUDE_PATH "${WL_3RDPARTY_DIR}/include;${WL_3RDPARTY_DIR}/include/sdl")
  set (CMAKE_LIBRARY_PATH "${WL_3RDPARTY_DIR}/lib")
endif (DEFINED WL_3RDPARTY_DIR)

find_package(GGZ_Core)
if (GGZ_CORE_FOUND)
  include_directories(${GGZ_CORE_INCLUDE_DIR})
  add_definitions("-DUSE_GGZ")
else (GGZ_CORE_FOUND)
  message (STATUS "GGZ support disabled, check your libraries path and installed packages!")
endif (GGZ_CORE_FOUND)

# ...but using MAJOR, MINOR and PATCH
if (DEFINED WL_VERSION_MAJOR)
	set (CPACK_PACKAGE_VERSION_MAJOR ${WL_VERSION_MAJOR})

	# Until 1.x.x set standard VERSION => "BuildXX"
	if (WL_VERSION_MAJOR STREQUAL "0")
		if (DEFINED WL_VERSION_MINOR)
			set (WL_VERSION Build${WL_VERSION_MINOR})
			set (CPACK_PACKAGE_VERSION_MINOR ${WL_VERSION_MINOR})
			if (DEFINED WL_VERSION_PATCH)
				set (WL_VERSION ${WL_VERSION}.${WL_VERSION_PATCH})
				set (CPACK_PACKAGE_VERSION_PATCH ${WL_VERSION_PATCH})
			else (DEFINED WL_VERSION_PATCH)
				set (CPACK_PACKAGE_VERSION_PATCH 0)
			endif (DEFINED WL_VERSION_PATCH)
			if (DEFINED WL_VERSION_ADDITION)
				set (WL_VERSION ${WL_VERSION}${WL_VERSION_ADDITION})
			endif (DEFINED WL_VERSION_ADDITION)
		else (DEFINED WL_VERSION_MINOR)
			set (WL_VERSION "NO BUILD ID")
		endif (DEFINED WL_VERSION_MINOR)

	# from 1.x.x on:
	else (WL_VERSION_MAJOR STREQUAL "0")
		set (WL_VERSION ${WL_VERSION_MAJOR})
		if (DEFINED WL_VERSION_MINOR)
			set (WL_VERSION ${WL_VERSION}.${WL_VERSION_MINOR})
			set (CPACK_PACKAGE_VERSION_MINOR ${WL_VERSION_MINOR})
			if (DEFINED WL_VERSION_PATCH)
				set (WL_VERSION ${WL_VERSION}.${WL_VERSION_PATCH})
				set (CPACK_PACKAGE_VERSION_PATCH ${WL_VERSION_PATCH})
			else (DEFINED WL_VERSION_PATCH)
				set (CPACK_PACKAGE_VERSION_PATCH 0)
			endif (DEFINED WL_VERSION_PATCH)
			if (DEFINED WL_VERSION_ADDITION)
				set (WL_VERSION ${WL_VERSION}${WL_VERSION_ADDITION})
			endif (DEFINED WL_VERSION_ADDITION)
		else (DEFINED WL_VERSION_MINOR)
			set (CPACK_PACKAGE_VERSION_MINOR 0)
		endif (DEFINED WL_VERSION_MINOR)
	endif (WL_VERSION_MAJOR STREQUAL "0")
endif (DEFINED WL_VERSION_MAJOR)

if (NOT DEFINED WL_VERSION)
  add_custom_target (
    BzrRevision ALL
    COMMAND cmake -DWL_INSTALL_PREFIX=${WL_INSTALL_PREFIX} -DWL_INSTALL_BINDIR=${WL_INSTALL_BINDIR} -DWL_INSTALL_DATADIR=${WL_INSTALL_DATADIR} -DWL_INSTALL_LOCALEDIR=${WL_INSTALL_LOCALEDIR} -DPYTHON_EXECUTABLE=${PYTHON_EXECUTABLE} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR} -DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR} -P ${CMAKE_CURRENT_SOURCE_DIR}/BzrRevision.cmake
  )
else (NOT DEFINED WL_VERSION)
  add_custom_target (
    InputRevision ALL
    COMMAND cmake -DWL_INSTALL_PREFIX=${WL_INSTALL_PREFIX} -DWL_INSTALL_BINDIR=${WL_INSTALL_BINDIR} -DWL_INSTALL_DATADIR=${WL_INSTALL_DATADIR} -DWL_INSTALL_LOCALEDIR=${WL_INSTALL_LOCALEDIR} -DWL_VERSION=${WL_VERSION} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR} -DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR} -P ${CMAKE_CURRENT_SOURCE_DIR}/InputRevision.cmake
  )
endif (NOT DEFINED WL_VERSION)

install (
  FILES ${CMAKE_CURRENT_BINARY_DIR}/VERSION
  DESTINATION ${WLBUILD_INSTALL_DATADIR}
  CONFIGURATIONS Debug;Release
  COMPONENT CoreVersionFile
)

include_directories(src ${CMAKE_CURRENT_BINARY_DIR}/src)

include(CheckIncludeFile)

find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIR})

IF (NOT WIN32)
  find_package(JPEG REQUIRED)
  include_directories(${JPEG_INCLUDE_DIR})

  find_package(TIFF REQUIRED)
  include_directories(${TIFF_INCLUDE_DIR})
ENDIF (NOT WIN32)

find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIR})

find_package(Lua51 5.1.5 REQUIRED)
include_directories(${LUA_INCLUDE_DIR})

if (APPLE)
  find_package(intl REQUIRED)
  include_directories(${INTL_INCLUDE_DIR})
endif (APPLE)

find_package(SDL_image REQUIRED)
include_directories(${SDLIMAGE_INCLUDE_DIR})

find_package(SDL_mixer REQUIRED)
include_directories(${SDLMIXER_INCLUDE_DIR})

find_package(SDL_net REQUIRED)
include_directories(${SDLNET_INCLUDE_DIR})

find_package(SDL_ttf REQUIRED)
include_directories(${SDLTTF_INCLUDE_DIR})

find_package(SDL_gfx REQUIRED)
include_directories(${SDLGFX_INCLUDE_DIR})

find_package(Lua51 REQUIRED)
include_directories(${LUA_INCLUDE_DIR})

IF (WIN32)
  SET(GUI_TYPE WIN32)
ENDIF (WIN32)

#configure_file (src/config.h.cmake src/config.h)
#configure_file (src/build_info.cc.cmake src/build_info.cc)

add_subdirectory(doc)
#add_subdirectory(po)
#add_subdirectory(pics)
add_subdirectory(src)
add_subdirectory(libext)

target_link_libraries(widelands ${Boost_LIBRARIES})

# install files to the correct locations here
install(
  DIRECTORY
    campaigns
    fonts
    global
    maps
    music
    sound
    tribes
    txts
    worlds
    pics
  DESTINATION ${WLBUILD_INSTALL_DATADIR}
  CONFIGURATIONS Debug;Release
  COMPONENT CoreDataFiles
  PATTERN "*.svn*" EXCLUDE
  PATTERN "SCons*" EXCLUDE
)

install(
  FILES
    COPYING
    CREDITS
    ChangeLog
  DESTINATION ${WLBUILD_INSTALL_DATADIR}
  CONFIGURATIONS Debug;Release
  COMPONENT CoreLicenseFiles
)

install(
  DIRECTORY
    doc
  DESTINATION ${WLBUILD_INSTALL_DATADIR}
  CONFIGURATIONS Debug
  COMPONENT DocFiles
  PATTERN "*.svn*" EXCLUDE
  PATTERN "SCons*" EXCLUDE
  PATTERN "CMakeLists.txt" EXCLUDE
)

find_package (PythonInterp REQUIRED)

if (WIN32)
  set (WL_CREATE_SYMLINK_COMMAND "copy_directory")
else (WIN32)
  set (WL_CREATE_SYMLINK_COMMAND "create_symlink")
endif (WIN32)

if (${CMAKE_BUILD_TYPE} STREQUAL Release)
  add_custom_target (
    lang ALL
    COMMAND cmake -E echo "Preparing directory po"
    COMMAND cmake -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/po ${CMAKE_CURRENT_BINARY_DIR}/po
    COMMAND cmake -E echo "Preparing directory campaigns"
    COMMAND cmake -E ${WL_CREATE_SYMLINK_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR}/campaigns ${CMAKE_CURRENT_BINARY_DIR}/campaigns
    COMMAND cmake -E echo "Preparing directory maps"
    COMMAND cmake -E ${WL_CREATE_SYMLINK_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR}/maps ${CMAKE_CURRENT_BINARY_DIR}/maps
    COMMAND cmake -E echo "Preparing directory tribes"
    COMMAND cmake -E ${WL_CREATE_SYMLINK_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR}/tribes ${CMAKE_CURRENT_BINARY_DIR}/tribes
    COMMAND cmake -E echo "Preparing directory txts"
    COMMAND cmake -E ${WL_CREATE_SYMLINK_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR}/txts ${CMAKE_CURRENT_BINARY_DIR}/txts
    COMMAND cmake -E echo "Preparing directory worlds"
    COMMAND cmake -E ${WL_CREATE_SYMLINK_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR}/worlds ${CMAKE_CURRENT_BINARY_DIR}/worlds
    COMMAND cmake -E echo "Running the locale generation task"
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/utils/buildlocale.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
else (${CMAKE_BUILD_TYPE} STREQUAL Release)
  add_custom_target (
    lang
    COMMAND cmake -E echo "Preparing directory po"
    COMMAND cmake -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/po ${CMAKE_CURRENT_BINARY_DIR}/po
    COMMAND cmake -E echo "Preparing directory campaigns"
    COMMAND cmake -E ${WL_CREATE_SYMLINK_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR}/campaigns ${CMAKE_CURRENT_BINARY_DIR}/campaigns
    COMMAND cmake -E echo "Preparing directory maps"
    COMMAND cmake -E ${WL_CREATE_SYMLINK_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR}/maps ${CMAKE_CURRENT_BINARY_DIR}/maps
    COMMAND cmake -E echo "Preparing directory tribes"
    COMMAND cmake -E ${WL_CREATE_SYMLINK_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR}/tribes ${CMAKE_CURRENT_BINARY_DIR}/tribes
    COMMAND cmake -E echo "Preparing directory txts"
    COMMAND cmake -E ${WL_CREATE_SYMLINK_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR}/txts ${CMAKE_CURRENT_BINARY_DIR}/txts
    COMMAND cmake -E echo "Preparing directory worlds"
    COMMAND cmake -E ${WL_CREATE_SYMLINK_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR}/worlds ${CMAKE_CURRENT_BINARY_DIR}/worlds
    COMMAND cmake -E echo "Running the locale generation task"
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/utils/buildlocale.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
  add_custom_target (
    lang_dummy ALL
    COMMAND cmake -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/locale
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
endif (${CMAKE_BUILD_TYPE} STREQUAL Release)

set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES locale)

install(
  DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/locale/
  DESTINATION ${WLBUILD_INSTALL_LOCALEDIR}
  CONFIGURATIONS Debug;Release
  COMPONENT CoreLanguageFiles
  PATTERN "*.svn*" EXCLUDE
  PATTERN "SCons*" EXCLUDE
)

execute_process (COMMAND cmake -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/pics)
add_custom_target (
  optimizepics 
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/pics
  COMMAND cmake -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DWL_CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}/pics -DWL_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/pics -P PicsFunctions.cmake
)

set (CPACK_PACKAGE_FILE_NAME widelands-${WL_VERSION})
set (CPACK_COMPONENTS_ALL ExecutableFiles CoreDataFiles DocFiles CoreLanguageFiles CoreVersionFile CoreLicenseFiles)
set (CPACK_GENERATOR TGZ)
set (CPACK_SOURCE_GENERATOR TGZ)
include(CPack)
