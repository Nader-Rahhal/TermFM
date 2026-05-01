cmake_minimum_required(VERSION 3.14)
project(terfm VERSION 1.2.0)
set(CMAKE_CXX_STANDARD 20)
include(FetchContent)

find_program(FFPLAY_EXECUTABLE ffplay)
if(NOT FFPLAY_EXECUTABLE)
    message(WARNING "ffplay not found — install ffmpeg and make sure ffplay is on your PATH")
else()
    message(STATUS "Found ffplay: ${FFPLAY_EXECUTABLE}")
endif()

find_package(CURL REQUIRED)

FetchContent_Declare(ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
    GIT_TAG main
)
FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp
    DOWNLOAD_NO_EXTRACT TRUE
)
FetchContent_MakeAvailable(ftxui json)

add_executable(termfm
    src/Main.cpp
    src/AudioPlayer.cpp
    src/RadioBrowser.cpp
)

target_compile_definitions(termfm PRIVATE APP_VERSION="${PROJECT_VERSION}")
target_link_libraries(termfm PRIVATE ftxui::component CURL::libcurl)
target_include_directories(termfm PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${json_SOURCE_DIR}
)