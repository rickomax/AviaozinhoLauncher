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
    set(GAMETITLEEX_VALUE "Aviãozinho do Tráfico Pré 4")
    set(PIPE_NAME_VALUE "\\\\\\\\.\\\\pipe\\\\Aviaozinho35")
    set(STEAM_APP_ID_VALUE 4241920)
else()
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

include_directories("include")
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/win64)

add_definitions(-DGAMEEXECUTABLE=\"AVIAO3GAME.EXE\")
add_definitions(-DGAMETITLEEX=\"${GAMETITLEEX_VALUE}\")
add_definitions(-DPIPE_NAME=\"${PIPE_NAME_VALUE}\")
add_definitions(-DNETPIPE_NAME=\"${NETPIPE_NAME_VALUE}\")
add_definitions(-DSTEAM_APP_ID=${STEAM_APP_ID_VALUE})

add_executable(AVIAO3
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

target_link_libraries(AVIAO3 steam_api64)

if(WIN32)
    if(MSVC)
        if(CONSOLE_BUILD)
            target_link_options(AVIAO3 PRIVATE "/SUBSYSTEM:CONSOLE")
        else()
            target_link_options(AVIAO3 PRIVATE "/SUBSYSTEM:WINDOWS")
        endif()
    else()
        if(CONSOLE_BUILD)
            target_link_options(AVIAO3 PRIVATE -mconsole)
        else()
            target_link_options(AVIAO3 PRIVATE -mwindows)
        endif()
    endif()
endif()

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/languages.txt DESTINATION ${OUTPUT_DIR}/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/bin/win64/ DESTINATION ${OUTPUT_DIR}/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/steam_appid.txt DESTINATION ${OUTPUT_DIR}/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/background.bmp DESTINATION ${OUTPUT_DIR}/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/QuakeSpasm.ico DESTINATION ${OUTPUT_DIR}/)

if(CMAKE_VERSION VERSION_GREATER 3.12)
    set_property(TARGET AVIAO3 PROPERTY CXX_STANDARD 20)
endif()