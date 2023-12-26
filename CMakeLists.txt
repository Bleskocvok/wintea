cmake_minimum_required(VERSION 3.5)

project(WinTea)

set(CMAKE_CXX_STANDARD 17)

set(WIN32_EECUTABLEX 1)

if(MSVC)
    add_compile_options(/W4 /Wall)
    # add_compile_options(/W4
    #                     /Wall
    #                     /Zi
    #                     /DEBUG:FULL
    #                     /MT
    #                     /EHsc
    #                     /Ob0
    #                     /Oy-
    #                     )
else()
    add_compile_options(-Wall -Wextra)
    # add_compile_options(-Wall -Wextra -g)
endif()

add_custom_target(resources.o ALL)


add_custom_command(OUTPUT resources.o
    COMMAND
        windres -i "${CMAKE_SOURCE_DIR}/resources.rc" -o resources.o
    DEPENDS
        "${CMAKE_SOURCE_DIR}/resources.rc"
)

add_executable(chat chat.cpp resources.o)

if(MSVC)
    add_executable(topwin WIN32 topwin.cpp resources.o)
else()
    add_executable(topwin topwin.cpp resources.o)
endif()

target_link_libraries(chat PRIVATE user32)
target_link_libraries(topwin PRIVATE user32)
