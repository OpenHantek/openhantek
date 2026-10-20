if(WIN32)
    # The application uses its bundled Windows libusb package in a sibling
    # directory. Keep default CTest configuration independent of a second
    # libusb discovery until this fixture is exercised on that toolchain.
    message(STATUS "Skipping DSO2250 test targets on Windows; the application build is unchanged")
    return()
endif()

find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(libusb REQUIRED)

set(DSO_TEST_SRC "${CMAKE_SOURCE_DIR}/openhantek/src")
set(DSO_TEST_CORE
    ${DSO_TEST_SRC}/hantekdso/hantekdsocontrol.cpp
    ${DSO_TEST_SRC}/hantekdso/hantekdsocontrol.h
    ${DSO_TEST_SRC}/hantekdso/controlsettings.cpp
    ${DSO_TEST_SRC}/hantekdso/controlspecification.cpp
    ${DSO_TEST_SRC}/hantekdso/dsomodel.cpp
    ${DSO_TEST_SRC}/hantekdso/modelregistry.cpp
    ${DSO_TEST_SRC}/hantekdso/models/modelDSO2250.cpp
    ${DSO_TEST_SRC}/hantekprotocol/bulkcommand.cpp
    ${DSO_TEST_SRC}/hantekprotocol/bulkStructs.cpp
    ${DSO_TEST_SRC}/hantekprotocol/controlcommand.cpp
    ${DSO_TEST_SRC}/hantekprotocol/controlStructs.cpp
    ${DSO_TEST_SRC}/usb/usbdevice.cpp
    ${DSO_TEST_SRC}/usb/usbdevice.h
    ${DSO_TEST_SRC}/usb/dso2250firmware.cpp
    ${DSO_TEST_SRC}/utils/printutils.cpp)

function(dso_test_setup target)
    set_target_properties(${target} PROPERTIES AUTOMOC ON)
    target_compile_features(${target} PRIVATE cxx_range_for)
    target_include_directories(${target} PRIVATE ${DSO_TEST_SRC} ${DSO_TEST_SRC}/hantekdso ${LIBUSB_INCLUDE_DIRS})
    foreach(usb_include IN LISTS LIBUSB_INCLUDE_DIRS)
        get_filename_component(usb_include_parent "${usb_include}" DIRECTORY)
        if(EXISTS "${usb_include_parent}/libusb-1.0/libusb.h")
            target_include_directories(${target} PRIVATE "${usb_include_parent}")
        endif()
    endforeach()
    target_link_libraries(${target} PRIVATE Qt5::Core Qt5::Widgets)
endfunction()

# This target defines every libusb symbol used by USBDevice. It deliberately
# does not link libusb, so the offline suite cannot open physical USB hardware.
add_executable(dso2250_protocol dso2250_protocol.cpp ${DSO_TEST_CORE}
    ${DSO_TEST_SRC}/hantekdso/models/modelDSO2090.cpp)
dso_test_setup(dso2250_protocol)
target_compile_definitions(dso2250_protocol PRIVATE
    DSO2250_FACTORY_HEX="${CMAKE_SOURCE_DIR}/openhantek/res/firmware/dso2250x86-firmware.hex")
add_test(NAME dso2250_protocol COMMAND dso2250_protocol)

find_package(FFTW REQUIRED)
add_executable(post_pipeline post_pipeline.cpp
    ${DSO_TEST_SRC}/post/postprocessing.cpp
    ${DSO_TEST_SRC}/post/postprocessing.h
    ${DSO_TEST_SRC}/post/ppresult.cpp
    ${DSO_TEST_SRC}/post/mathchannelgenerator.cpp
    ${DSO_TEST_SRC}/post/spectrumgenerator.cpp
    ${DSO_TEST_SRC}/post/graphgenerator.cpp
    ${DSO_TEST_SRC}/post/graphgenerator.h
    ${DSO_TEST_SRC}/post/softwaretrigger.cpp)
dso_test_setup(post_pipeline)
target_include_directories(post_pipeline PRIVATE ${FFTW_INCLUDE_DIRS})
target_link_libraries(post_pipeline PRIVATE ${FFTW_LIBRARIES})
add_test(NAME post_pipeline COMMAND post_pipeline)

option(BUILD_HARDWARE_TESTS "Build manually invoked DSO2250 physical hardware probe" OFF)
if(BUILD_HARDWARE_TESTS)
    add_executable(dso2250_hardware dso2250_hardware.cpp ${DSO_TEST_CORE}
        ${DSO_TEST_SRC}/usb/uploadFirmware.cpp
        ${DSO_TEST_SRC}/usb/ezusb.cpp
        ${CMAKE_SOURCE_DIR}/openhantek/res/firmwares.qrc)
    dso_test_setup(dso2250_hardware)
    set_target_properties(dso2250_hardware PROPERTIES AUTORCC ON)
    target_link_libraries(dso2250_hardware PRIVATE ${LIBUSB_LIBRARIES})
    # Never register the hardware probe with CTest; it is run explicitly.
endif()
