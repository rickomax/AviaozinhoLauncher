cmake_minimum_required (VERSION 3.8)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("AVIAO3")

include_directories("include")
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/win64)

add_definitions(-DGAMEEXECUTABLE=\"AVIAO3GAME.EXE\")
add_definitions(-DSTEAM_APP_ID=3191050)
add_definitions(-DTESTES)

add_executable (AVIAO3 "Pipe.c" "Pipe.h" "Game.h" "LauncherGUI.h" "LauncherGUI.cpp" "Request.h" resource.rc)

target_link_libraries(AVIAO3 steam_api64)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/bin/win64/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/steam_appid.txt DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/background.bmp DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/QuakeSpasm.ico DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET AVIAO3 PROPERTY CXX_STANDARD 20)
endif()
