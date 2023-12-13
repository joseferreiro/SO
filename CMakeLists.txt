cmake_minimum_required(VERSION 3.26)
project(untitled C)

set(CMAKE_C_STANDARD 99)

add_executable(untitled main.c
        lista_comandos.h
        lista_comandos.c
        lista_ficheros.h
        lista_ficheros.c
        lista_memoria.h
        lista_memoria.c
        lista_procesos.h
        lista_procesos.c
        comandos_P3.h
        comandos_P3.c
        comandos_P0.h
        comandos_P0.c
)
