project(FirmwareExtractor C)

cmake_minimum_required(VERSION 3.9)
# works with mingw compiler
if(WIN32)
    find_library( BFD_LIBRARY bfd PATH 
        /MinGW/lib)
else()
    find_library( BFD_LIBRARY NAMES bfd libbdf PATH 
        /usr/lib
        /usr/lib64)
endif()
find_path(BFD_INCLUDE bfd.h PATHS 
    /usr/include
    /usr/local/include
    /opt/local/include
    /MinGW/include)

message(${BFD_INCLUDE})
message(${BFD_LIBRARY})
if (NOT BFD_LIBRARY OR NOT BFD_INCLUDE)
    message(STATUS "BFD not found. Please install binutils-devel (fedora) / binutils-dev (ubuntu) / mingw32-binutils-dev (windows)")
    return()
endif()

add_executable(${PROJECT_NAME} extractfw.c)
target_link_libraries(${PROJECT_NAME} ${BFD_LIBRARY} )
target_include_directories(${PROJECT_NAME} PRIVATE ${BFD_INCLUDE})