if (NOT WIN32)
    return()
endif()

macro( CheckExitCodeAndExitIfError MSG)
  if(NOT ${ExitCode} EQUAL 0)
    message(FATAL_ERROR "Failed: ${MSG}")
    return(${ExitCode})
  endif()
endmacro( CheckExitCodeAndExitIfError )

set(filename "${CMAKE_BINARY_DIR}/fftw.zip")

if (CMAKE_SIZEOF_VOID_P EQUAL 4)
    message("Download/Extract FFTW for 32bit")
    if (NOT EXISTS "${CMAKE_BINARY_DIR}/fftw.zip")
        # file(DOWNLOAD "ftp://ftp.fftw.org/pub/fftw/fftw-3.3.5-dll32.zip" "${filename}" SHOW_PROGRESS)
        set(filename "${CMAKE_CURRENT_LIST_DIR}/fftw-3.3.5-dll32.zip")
    endif()
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(LIBEXE_64 "/machine:x64")
    message("Download/Extract FFTW for 64bit")
    if (NOT EXISTS "${CMAKE_BINARY_DIR}/fftw.zip")
        # file(DOWNLOAD "ftp://ftp.fftw.org/pub/fftw/fftw-3.3.5-dll64.zip" "${filename}" SHOW_PROGRESS)
        set(filename "${CMAKE_CURRENT_LIST_DIR}/fftw-3.3.5-dll64.zip")
    endif()
else()
    message(FATAL_ERROR "Target architecture not known")
endif()

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/fftw")

execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar xzf "${filename}"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/fftw"
    RESULT_VARIABLE ExitCode
)
CheckExitCodeAndExitIfError("tar")

get_filename_component(_vs_bin_path "${CMAKE_LINKER}" DIRECTORY)

# use dlltool in case of system is unix and target system is windows
# not sure about binary name, maybe it can be differ on some distros. 
# i686-w64-mingw32-dlltool this one is correct for fedora
# in the same way it's possible to add compiling with mingw on windows, just need exe name.
if(CMAKE_HOST_UNIX AND WIN32)
     execute_process(
	COMMAND i686-w64-mingw32-dlltool ${LIBEXE_64} -d ${CMAKE_BINARY_DIR}/fftw/libfftw3-3.def -l ${CMAKE_BINARY_DIR}/fftw/libfftw3-3.lib
	WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/fftw"
	OUTPUT_VARIABLE OutVar
	ERROR_VARIABLE ErrVar
	RESULT_VARIABLE ExitCode)
    CheckExitCodeAndExitIfError("i686-w64-mingw32-dlltool: ${OutVar} ${ErrVar}")
else()
    execute_process(
	COMMAND "${_vs_bin_path}/lib.exe" ${LIBEXE_64} /def:${CMAKE_BINARY_DIR}/fftw/libfftw3-3.def /out:${CMAKE_BINARY_DIR}/fftw/libfftw3-3.lib
	WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/fftw"
	OUTPUT_VARIABLE OutVar
	ERROR_VARIABLE ErrVar
	RESULT_VARIABLE ExitCode)
    CheckExitCodeAndExitIfError("lib.exe: ${OutVar} ${ErrVar}")
endif()


target_link_libraries(${PROJECT_NAME} "${CMAKE_BINARY_DIR}/fftw/libfftw3-3.lib")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_BINARY_DIR}/fftw")

file(COPY "${CMAKE_BINARY_DIR}/fftw/fftw3.h" DESTINATION "${CMAKE_SOURCE_DIR}/src")

add_custom_command(TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_BINARY_DIR}/fftw/libfftw3-3.dll" $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copy fftw3 dlls for ${PROJECT_NAME}"
)

