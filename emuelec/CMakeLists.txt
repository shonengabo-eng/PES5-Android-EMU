# Etapa 1a para EmuELEC (S905X3, aarch64): una sonda que abre una ventana SDL2 con contexto GLES,
# imprime lo que ofrece la Mali-G31, dibuja un triangulo y lee el mando. No toca el resto del proyecto.
# Se configura con: cmake -S emuelec -B build-emuelec -DCMAKE_TOOLCHAIN_FILE=emuelec-aarch64.cmake
cmake_minimum_required(VERSION 3.16)
project(pes5_emuelec C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SDL2 2.0.9 de Debian buster desempaquetado por el workflow
set(SDL2_ROOT /opt/sdl2dev CACHE PATH "SDL2 2.0.9 (cabeceras y libreria)")

add_executable(pes5_gles_probe gles_probe.cpp)
target_include_directories(pes5_gles_probe SYSTEM PRIVATE
    ${SDL2_ROOT}/usr/include/SDL2
    ${SDL2_ROOT}/usr/include/aarch64-linux-gnu
    ${SDL2_ROOT}/usr/include)
# se enlaza contra la .so.0 directamente (el enlace libSDL2.so del paquete -dev apunta fuera del sysroot);
# las funciones GL no se enlazan: se piden en ejecucion con SDL_GL_GetProcAddress a la libMali de la caja
target_link_libraries(pes5_gles_probe PRIVATE ${SDL2_ROOT}/usr/lib/aarch64-linux-gnu/libSDL2-2.0.so.0)
target_link_options(pes5_gles_probe PRIVATE -Wl,--allow-shlib-undefined)
