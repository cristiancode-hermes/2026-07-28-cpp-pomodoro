cmake_minimum_required(VERSION 3.28)
project(pomodoro VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Sources ──
file(GLOB_RECURSE SOURCES src/*.cpp)
file(GLOB_RECURSE HEADERS include/*.hpp include/*.h)

add_executable(pomodoro ${SOURCES} ${HEADERS})
target_include_directories(pomodoro PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# ── Install ──
install(TARGETS pomodoro DESTINATION bin)
