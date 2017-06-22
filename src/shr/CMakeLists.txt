cmake_minimum_required(VERSION 2.8)

project(shr)

find_package (Boost COMPONENTS date_time program_options REQUIRED)

set(EXE_SOURCES
    main.cpp
)

include_directories(
    
)

add_executable(${PROJECT_NAME} ${EXE_SOURCES})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)

target_link_libraries (${PROJECT_NAME}
    bb_api
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
)

