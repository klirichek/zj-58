cmake_minimum_required ( VERSION 3.12 )

# set default build type to Release (if nothing else provided from outside yet)
IF ( NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES )
	SET ( CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE )
	set_property ( CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
			"MinSizeRel" "RelWithDebInfo" )
ENDIF ()

project ( rastertozj C )

#set ( CMAKE_INSTALL_PREFIX "/usr" CACHE FILEPATH "force install to /usr" FORCE )

function( DIAG VARR )
	message ( STATUS "${VARR} -> ${${VARR}}" )
endfunction()

# build the filter binary
find_package ( Cups REQUIRED )
find_library ( CUPSIMAGELIB NAMES cupsimage )
mark_as_advanced ( CUPSIMAGELIB )

set ( DEBUGFILE "/tmp/debugraster.txt" CACHE STRING "File where to make debug output if DEBUGP is set" )
option ( DEBUGP "Whetner to output debug info" ON )

add_executable ( rastertozj rastertozj.c )
target_compile_options ( rastertozj PRIVATE -Wall -fPIC )
target_include_directories ( rastertozj PRIVATE ${CUPS_INCLUDE_DIR} )
target_link_libraries ( rastertozj PRIVATE ${CUPS_LIBRARIES} ${CUPSIMAGELIB} )

if ( DEBUGP )
	target_compile_definitions ( rastertozj PRIVATE DEBUGP DEBUGFILE="${DEBUGFILE}")
endif ()

# build ppds
if ( NOT DEFINED PPDC )
	find_program ( PPDC ppdc )
endif ()
mark_as_advanced( PPDC )

if ( PPDC )
	set ( ENV{LANG} c )
	execute_process (
			COMMAND "${PPDC}" -d "${CMAKE_CURRENT_BINARY_DIR}" "zj-58.drv"
			WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
			RESULT_VARIABLE res
			OUTPUT_VARIABLE _CONTENT
			ERROR_QUIET
			OUTPUT_STRIP_TRAILING_WHITESPACE )
endif()

# installation stuff

# this one works if you perform 'make install'
set ( CMAKE_INSTALL_PREFIX "/usr" CACHE FILEPATH "this is forced to /usr" FORCE )

# this one works if you perform 'make package' - it is kind of root for all packaging
set ( CPACK_PACKAGING_INSTALL_PREFIX "/" )


include ( GNUInstallDirs )

if ( APPLE )
	set ( FILTERPATH "usr/libexec/cups/filter" )
	set ( PPDPATH "Library/Printers/PPDs/Contents/Resources")
elseif ( UNIX )
	set ( FILTERPATH "usr/lib/cups/filter" )
	set ( PPDPATH "usr/share/cups/model/zjiang/" )
endif()

install ( TARGETS rastertozj DESTINATION ${FILTERPATH} )
install ( FILES ${CMAKE_CURRENT_BINARY_DIR}/zj58.ppd DESTINATION "${PPDPATH}" )
include (CPack)

