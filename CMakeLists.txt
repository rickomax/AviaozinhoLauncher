cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project("AVIAO3")

option(CONSOLE_BUILD "Enables console" OFF)
option(NO_LAUNCHER "Disables launcher" OFF)
option(STEAM_TEST "Disables Steam game validation" OFF)
option(STEAM_LOCAL_TEST "Enables Steam Relay IP connection" OFF)
option(BDD_PRE_4 "Enable BDD PRE-4 settings" OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR})

if(BDD_PRE_4)
    set(EXECUTABLE_NAME "AVIAO4")
    set(GAMETITLEEX_VALUE "Aviãozinho do Tráfico Pré 4")
    set(PIPE_NAME_VALUE "\\\\\\\\.\\\\pipe\\\\Aviaozinho35")
    set(STEAM_APP_ID_VALUE 4241920)
else()
    set(EXECUTABLE_NAME "AVIAO3")
    set(GAMETITLEEX_VALUE "Aviãozinho do Tráfico 3")
    set(PIPE_NAME_VALUE "\\\\\\\\.\\\\pipe\\\\Aviaozinho")
    set(STEAM_APP_ID_VALUE 3191050)
endif()

if(STEAM_TEST)
    add_definitions(-DSTEAM_TEST)
endif()

if(CONSOLE_BUILD)
    add_definitions(-DCONSOLE_BUILD)
endif()

if(NO_LAUNCHER)
    add_definitions(-DNO_LAUNCHER)
endif()

if(STEAM_LOCAL_TEST)
    add_definitions(-DLOCAL_TEST)
endif()

if(BDD_PRE_4)
    add_definitions(-DBDDPRE4)
endif()

set(STEAM_PUBLIC_DIR "" CACHE PATH "Path to the Steam API public headers directory (contains the 'steam' folder)")
set(STEAM_LIB_DIR "" CACHE PATH "Path to the Steam API library directory (contains the steam_api64 import library)")

if(NOT STEAM_PUBLIC_DIR)
    message(FATAL_ERROR "STEAM_PUBLIC_DIR is not set. Configure with -DSTEAM_PUBLIC_DIR=<path to the Steam API public headers directory>.")
endif()

if(NOT IS_DIRECTORY "${STEAM_PUBLIC_DIR}")
    message(FATAL_ERROR "STEAM_PUBLIC_DIR does not point to an existing directory: ${STEAM_PUBLIC_DIR}")
endif()

if(NOT STEAM_LIB_DIR)
    message(FATAL_ERROR "STEAM_LIB_DIR is not set. Configure with -DSTEAM_LIB_DIR=<path to the Steam API library directory>.")
endif()

if(NOT IS_DIRECTORY "${STEAM_LIB_DIR}")
    message(FATAL_ERROR "STEAM_LIB_DIR does not point to an existing directory: ${STEAM_LIB_DIR}")
endif()

include_directories("${STEAM_PUBLIC_DIR}")
link_directories("${STEAM_LIB_DIR}")

add_definitions(-DGAMEEXECUTABLE=\"AVIAO3GAME.EXE\")
add_definitions(-DGAMETITLEEX=\"${GAMETITLEEX_VALUE}\")
add_definitions(-DPIPE_NAME=\"${PIPE_NAME_VALUE}\")
add_definitions(-DNETPIPE_NAME=\"${NETPIPE_NAME_VALUE}\")
add_definitions(-DSTEAM_APP_ID=${STEAM_APP_ID_VALUE})

add_executable(${EXECUTABLE_NAME}
    resource.rc
    Pipe.c
    LauncherGUI.cpp
    Uploader.cpp
    Downloader.cpp
    WorkshopUploadDialog.cpp
    Static.cpp
    SteamCallbacks.cpp
    NetBackend.cpp
)

target_link_libraries(${EXECUTABLE_NAME} steam_api64)

if(WIN32)
    if(MSVC)
        if(CONSOLE_BUILD)
            target_link_options(${EXECUTABLE_NAME} PRIVATE "/SUBSYSTEM:CONSOLE")
        else()
            target_link_options(${EXECUTABLE_NAME} PRIVATE "/SUBSYSTEM:WINDOWS")
        endif()
    else()
        if(CONSOLE_BUILD)
            target_link_options(${EXECUTABLE_NAME} PRIVATE -mconsole)
        else()
            target_link_options(${EXECUTABLE_NAME} PRIVATE -mwindows)
        endif()
    endif()
endif()

if(CMAKE_VERSION VERSION_GREATER 3.12)
    set_property(TARGET ${EXECUTABLE_NAME} PROPERTY CXX_STANDARD 20)
endif()