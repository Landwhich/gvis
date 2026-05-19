cmake_minimum_required(VERSION 3.16)
project(MyApp)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets) # Use Qt5 if needed
set(CMAKE_AUTOMOC ON)
add_executable(MyApp main.cpp)
target_link_libraries(MyApp PRIVATE Qt6::Core Qt6::Widgets)   
