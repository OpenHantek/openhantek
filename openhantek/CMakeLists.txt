project(OpenHantek CXX)

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets PrintSupport 3DCore 3DExtras 3DRender 3DInput)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (Qt5Widgets_VERSION VERSION_LESS 5.7.0)
    message(FATAL_ERROR "Minimum supported Qt5 version is 5.7.0!")
endif()

# include directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(src/ src/hantekdso src/settings) # src/widgets src/scopeview

# collect sources and other files
file(GLOB_RECURSE SRC_CORE
    "src/hantekdso/*.cpp" "src/hantekprotocol/*.cpp" "src/hantekprotocol/*.cpp" "src/usb/*.cpp" "src/utils/*.cpp"
    "src/hantekdso/*.h"   "src/hantekprotocol/*.h"   "src/hantekprotocol/*.h"   "src/usb/*.h"   "src/utils/*.h")
file(GLOB_RECURSE SRC_POST "src/post/*.cpp" "src/post/*.h")
file(GLOB_RECURSE SRC_SETTINGS  "src/settings/*.cpp" "src/settings/*.h")
file(GLOB_RECURSE SRC_EXPORT  "src/exporting/*.cpp" "src/exporting/*.h")
file(GLOB_RECURSE SRC_SCOPEVIEW  "src/scopeview/*.cpp" "src/scopeview/*.h")
file(GLOB SRC_UI "src/main.cpp"
    "src/widgets/*.cpp" "src/widgets/*.h"
    "src/mainwindow.cpp" "src/mainwindow.h"
    "src/configdialog/*.cpp" "src/configdialog/*.h"
    "src/docks/*.cpp" "src/docks/*.h"
    "src/iconfont/*.cpp" "src/iconfont/*.h"
    "src/selectdevice/*.cpp" "src/selectdevice/*.h")
set(SRC ${SRC_CORE} ${SRC_POST} ${SRC_SETTINGS} ${SRC_EXPORT} ${SRC_SCOPEVIEW} ${SRC_UI})

file(GLOB_RECURSE UI "src/*.ui")
file(GLOB_RECURSE QRC "res/*.qrc")

add_custom_target(format SOURCES ".clang-format"
    COMMAND "clang-format" "-style=file" "-i" "-sort-includes" ${SRC})

add_subdirectory(translations)

add_definitions(-DVERSION="${CPACK_PACKAGE_VERSION}" -Wall)

# make executable
add_executable(${PROJECT_NAME} ${SRC} ${UI} ${QRC} ${TRANSLATION_BIN_FILES} ${TRANSLATION_QRC})
target_link_libraries(${PROJECT_NAME}
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::3DCore
    Qt5::3DExtras
    Qt5::3DRender
    Qt5::3DInput
    Qt5::PrintSupport)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_range_for)
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE "/W4" "/wd4251" "/wd4127" "/wd4275" "/wd4200" "/nologo" "/J" "/Zi")
    target_compile_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:DEBUG>:/MDd>")
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wno-long-long -pedantic)
    target_compile_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:DEBUG>:-DDEBUG>")
    target_compile_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:DEBUG>:-O0>")
    target_compile_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:RELEASE>:-fno-rtti>")
endif()

include(../cmake/fftw_on_windows.cmake)
include(../cmake/libusb_on_windows.cmake)

if(NOT WIN32)
    find_package(libusb REQUIRED)
    target_include_directories(${PROJECT_NAME} PRIVATE ${LIBUSB_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} ${LIBUSB_LIBRARIES})

    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})

    find_package(FFTW REQUIRED)
    target_include_directories(${PROJECT_NAME} PRIVATE ${FFTW_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} ${FFTW_LIBRARIES})
endif()

# install commands
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "bin")

include(../cmake/copy_qt5_dlls_to_bin_dir.cmake)
