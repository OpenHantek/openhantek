project(OpenHantek CXX)

if(APPLE)
    list(APPEND CMAKE_CXX_FLAGS "-mmacosx-version-min=10.9 ${CMAKE_CXX_FLAGS}")
    # Default brew QT location
    set(QT5_LOCATION "/usr/local/opt/qt5" CACHE)
	if(EXISTS ${QT5_LOCATION})
	    list(APPEND CMAKE_PREFIX_PATH ${QT5_LOCATION})
	    set(MAC_DEPLOY_QT_TOOL "${QT5_LOCATION}/bin/macdeployqt")
	else()
	     message(FATAL_ERROR "Install brew and Qt5 by running:\n brew install qt5 \n(Optionally you can change cmake ../ -DQT5_LOCATION=<yourQt> variable to your own Qt version)")
	endif()
endif()



find_package(Qt5Widgets REQUIRED)
find_package(Qt5PrintSupport REQUIRED)
find_package(Qt5OpenGL REQUIRED)
find_package(OpenGL)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if (Qt5Widgets_VERSION VERSION_LESS 5.4.0)
    message(FATAL_ERROR "Minimum supported Qt5 version is 5.4.0!")
endif()

# include directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(src/ src/hantekdso src/widgets src/docks src/configdialog)

# collect sources and other files
file(GLOB_RECURSE SRC "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.h")
file(GLOB_RECURSE UI "src/*.ui")
file(GLOB_RECURSE QRC "res/*.qrc")

add_custom_target(format SOURCES ".clang-format"
    COMMAND "clang-format" "-style=file" "-i" "-sort-includes" ${SRC} ${HEADERS})

add_subdirectory(translations)

add_definitions(-DVERSION="${CPACK_PACKAGE_VERSION}")

set(EXECTYPE "")
if(MINGW)
    set(EXECTYPE WIN32)
elseif(APPLE)
    set(EXECTYPE MACOSX_BUNDLE)
endif()

# make executable
add_executable(${PROJECT_NAME} ${EXECTYPE} ${SRC} ${HEADERS} ${UI} ${QRC} ${TRANSLATION_BIN_FILES} ${TRANSLATION_QRC})
if(APPLE)
    set(MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME})
    set(MACOSX_BUNDLE_EXECUTABLE start.sh)
    set(MACOSX_BUNDLE_INFO_STRING "${PROJECT_NAME} Osciloscope Software")
    set(MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_NAME}")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_NAME}")
    set(MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_NAME}")
    set(MACOSX_BUNDLE_ICON_FILE openhantek.icns)
endif()
target_link_libraries(${PROJECT_NAME} Qt5::Widgets Qt5::PrintSupport Qt5::OpenGL ${OPENGL_LIBRARIES} )
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



# install command
if(NOT APPLE)
    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "bin")
elseif(APPLE)
    install(TARGETS ${PROJECT_NAME} BUNDLE DESTINATION .)
    if(EXISTS ${MAC_DEPLOY_QT_TOOL}
	add_custom_command(
	TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/openhantek/res/images/openhantek.icns" "${CMAKE_BINARY_DIR}/openhantek/${PROJECT_NAME}.app/Contents/Resources/openhantek.icns"
	COMMAND "${MAC_DEPLOY_QT_TOOL}" "${PROJECT_NAME}.app" "-verbose=3"
	COMMAND printf "'#!/bin/bash\\n'" > "${CMAKE_BINARY_DIR}/openhantek/${PROJECT_NAME}.app/Contents/MacOS/start.sh"
	COMMAND printf "cd '\"$$\{0%%/*}\"\\n'" >> "${CMAKE_BINARY_DIR}/openhantek/${PROJECT_NAME}.app/Contents/MacOS/start.sh"
	COMMAND printf "'./${PROJECT_NAME}\\n'" >> "${CMAKE_BINARY_DIR}/openhantek/${PROJECT_NAME}.app/Contents/MacOS/start.sh"
	COMMAND chmod 777 "${CMAKE_BINARY_DIR}/openhantek/${PROJECT_NAME}.app/Contents/MacOS/start.sh"
	COMMAND plutil -replace CFBundleExecutable -string start.sh "${CMAKE_BINARY_DIR}/openhantek/${PROJECT_NAME}.app/Contents/Info.plist"
	WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/openhantek"
	COMMENT "Copy app icon to bundle app")
    else()
	message(STATUS "No ${MAC_DEPLOY_QT_TOOL} found. So no dbundle.")
    endif()
endif()

#add_custom_target("deploy")

include(../cmake/copy_qt5_dlls_to_bin_dir.cmake)
