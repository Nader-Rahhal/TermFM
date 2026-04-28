cmake_minimum_required(VERSION 3.14)
project(radio)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

FetchContent_Declare(ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
    GIT_TAG main
)

FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp
    DOWNLOAD_NO_EXTRACT TRUE
)

FetchContent_MakeAvailable(ftxui json)

add_executable(radio src/Main.cpp)
target_link_libraries(radio PRIVATE ftxui::component)
target_include_directories(radio PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${json_SOURCE_DIR}
)