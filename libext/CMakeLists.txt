set(EXTLIB_INSTALL_DIR ${WLBUILD_INSTALL_DATADIR}/lib)
IF (WIN32)
  set(EXTLIB_INSTALL_DIR ${WLBUILD_INSTALL_BINDIR})
ENDIF (WIN32)

IF (USE_SDL12_COMPAT)
  include("sdl12-compat.cmake")
  target_include_directories(widelands PUBLIC "${install_dir}/include")
  target_link_libraries(widelands sdl12_compat)
  
  IF (USE_SDL12_SHARED)
    install(
      FILES 
        ${SDL_LIBRARY_FILES} 
      DESTINATION "${EXTLIB_INSTALL_DIR}"
      CONFIGURATIONS Debug;Release
      COMPONENT CoreDataFiles
    )
  ENDIF ()
ELSE (USE_SDL12_COMPAT)
  find_package(SDL REQUIRED)
  include_directories(${SDL_INCLUDE_DIR})
  target_link_libraries(widelands ${SDL_LIBRARY})
ENDIF (USE_SDL12_COMPAT)