cmake_minimum_required(VERSION 3.14)

project(zmqPoll LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(cppzmq REQUIRED)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core REQUIRED)
find_package(spdlog REQUIRED)
find_package(Threads REQUIRED)
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})
add_executable(zmqPoll
  main.cpp
)
add_executable(publisher publisher.cpp )
add_executable(subscriber subscriber.cpp)

target_link_libraries(publisher Qt${QT_VERSION_MAJOR}::Core cppzmq Threads::Threads )
target_link_libraries(subscriber cppzmq Threads::Threads)
target_link_libraries(zmqPoll Qt${QT_VERSION_MAJOR}::Core cppzmq Threads::Threads)
