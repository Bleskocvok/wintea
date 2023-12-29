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
        "${CMAKE_SOURCE_DIR}/cajik.ico"
)

# add_executable(chat chat.cpp resources.o)

if(MSVC)
    add_executable(tea WIN32 tea.cpp resources.o)
else()
    add_executable(tea tea.cpp resources.o)
endif()

# target_link_libraries(chat PRIVATE user32)
target_link_libraries(tea PRIVATE user32)
