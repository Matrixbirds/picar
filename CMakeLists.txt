cmake_minimum_required (VERSION 3.1)

set(APP_NAME pikapi)

set(CMAKE_CXX_COMPILER g++)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project (${APP_NAME})

include_directories(${CMAKE_SOURCE_DIR}/include)
link_directories(${CMAKE_SOURCE_DIR}/libs)

set (EXTRA_LIBS agora_rtm_sdk wiringPi)

set (EXTRA_LIBS ${EXTRA_LIBS})

add_executable(${APP_NAME} src/rtm_client.cpp)
target_link_libraries (${APP_NAME} ${EXTRA_LIBS})