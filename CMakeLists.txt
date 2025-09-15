cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project ("AVIAO3")

option(TEST_BUILD "Build local test version" OFF)
option(STEAM_TEST "Disables Steam game validation" ON)
option(CONSOLE_BUILD "Enables console" ON)

#todo: make these paths relative to the CMAKE project ahd remove hand-made values
if(TEST_BUILD)
    add_definitions(-DCURRENT_DIRECTORY=\"C:/Repos/spikedão/Windows/VisualStudio\")
    add_definitions(-DRELATIVE_BASEDIR=\"../../../\")
    set(OUTPUT_DIR "C:/Repos/spikedão/Windows/VisualStudio/Build-quakespasm-sdl2/x64/${CMAKE_BUILD_TYPE}")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR})
else()
    add_definitions(-DCURRENT_DIRECTORY=NULL)
    add_definitions(-DRELATIVE_BASEDIR=\"\")
endif()

if(STEAM_TEST)
    add_definitions(-DSTEAM_TEST)
endif()

if (CONSOLE_BUILD)
    add_definitions(-DCONSOLE_BUILD)
endif()

include_directories("include")
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/win64)

add_definitions(-DGAMEEXECUTABLE=\"AVIAO3GAME.EXE\")
add_definitions(-DREGISTRYKEY=L\"Software\\Joeveno\\Aviao\")
add_definitions(-DPIPE_NAME=\"\\\\\\\\.\\\\pipe\\\\Aviaozinho\")
add_definitions(-DSTEAM_APP_ID=3191050)

add_executable (AVIAO3 "Pipe.c" "LauncherGUI.cpp" "Uploader.cpp" "Downloader.cpp" "WorkshopUploadDialog.cpp" resource.rc)

target_link_libraries(AVIAO3 steam_api64)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/languages.txt DESTINATION ${OUTPUT_DIR}/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/bin/win64/ DESTINATION ${OUTPUT_DIR}/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/steam_appid.txt DESTINATION ${OUTPUT_DIR}/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/background.bmp DESTINATION ${OUTPUT_DIR}/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/QuakeSpasm.ico DESTINATION ${OUTPUT_DIR}/)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET AVIAO3 PROPERTY CXX_STANDARD 20)
endif()
