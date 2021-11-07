include(ExternalProject)

ExternalProject_Add(project_sdl12_compat
  PREFIX            sdl12_compat
  SOURCE_DIR        "${CMAKE_CURRENT_SOURCE_DIR}/sdl12-compat"
  INSTALL_DIR       "${CMAKE_CURRENT_BINARY_DIR}/sdl12_compat"
  STEP_TARGETS      build
  BUILD_ALWAYS      OFF
  CMAKE_ARGS        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
                    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
  CMAKE_CACHE_ARGS
        -DSDL12TESTS:BOOL=OFF
        -DSDL12DEVEL:BOOL=ON
        -DSTATICDEVEL:BOOL=ON   

  EXCLUDE_FROM_ALL TRUE
)

ExternalProject_Get_Property(project_sdl12_compat install_dir)

if (WIN32)
  set(SDL_LIBRARY_SHARED "${install_dir}/lib/libSDL.dll")
  FILE(GLOB SDL_LIBRARY_FILES "${install_dir}/lib/libSDL*.dll")
else()
  set(SDL_LIBRARY_SHARED "${install_dir}/lib/libSDL.so")
  FILE(GLOB SDL_LIBRARY_FILES "${install_dir}/lib/libSDL*.so*")
endif()
set(SDL_LIBRARY_STATIC "${install_dir}/lib/libSDL.a")

if(USE_SDL12_SHARED)
    add_library(sdl12_compat SHARED IMPORTED)
    set_property(TARGET sdl12_compat PROPERTY IMPORTED_LOCATION "${SDL_LIBRARY_SHARED}")
else()
    add_library(sdl12_compat STATIC IMPORTED)
    set_property(TARGET sdl12_compat PROPERTY IMPORTED_LOCATION "${SDL_LIBRARY_STATIC}")
endif()

add_dependencies(sdl12_compat project_sdl12_compat)
target_link_libraries(sdl12_compat INTERFACE ${CMAKE_DL_LIBS})

