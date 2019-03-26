cmake_minimum_required ( VERSION 3.0 )

# set default build type to Release (if nothing else provided from outside yet)
IF ( NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES )
	SET ( CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE )
	set_property ( CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
			"MinSizeRel" "RelWithDebInfo" )
ENDIF ()

project ( rastertozj C )

function( DIAG VARR )
	message ( STATUS "${VARR} -> ${${VARR}}" )
endfunction()

# requirements to build the filter binary
find_package ( Cups REQUIRED )
find_library ( CUPSIMAGELIB NAMES cupsimage )
mark_as_advanced ( CUPSIMAGELIB )

add_executable ( rastertozj rastertozj.c )
target_compile_options ( rastertozj PRIVATE -Wall -fPIC )
target_include_directories ( rastertozj PRIVATE ${CUPS_INCLUDE_DIR} )
target_link_libraries ( rastertozj PRIVATE ${CUPS_LIBRARIES} ${CUPSIMAGELIB} )

if ( CMAKE_BUILD_TYPE STREQUAL "Debug" )
	target_compile_definitions ( rastertozj PRIVATE DEBUGP )
	if ( APPLE )
		target_compile_definitions ( rastertozj PRIVATE SAFEDEBUG )
		message ( STATUS "On Mac OSX log is saved in sandbox, as /private/var/spool/cups/tmp" )
	elseif ( UNIX )
		set ( DEBUGFILE "/tmp/debugraster.txt" CACHE STRING "File where to make debug output if we build ad Debug" )
		target_compile_definitions ( rastertozj PRIVATE DEBUGFILE="${DEBUGFILE}" )
	endif ()
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
			ERROR_QUIET
			OUTPUT_STRIP_TRAILING_WHITESPACE )
endif()

# installation stuff

# Set both installation prefixes to /
set ( CMAKE_INSTALL_PREFIX "/" CACHE FILEPATH "this is forced to /" FORCE )
set ( CPACK_PACKAGING_INSTALL_PREFIX "/" )
set ( CPACK_PACKAGE_CONTACT "Alexey N. Vinogradov (a.n.vinogradov@gmail.com)" )

set ( CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON )
set ( CPACK_DEBIAN_PACKAGE_PREDEPENDS "libcups2-dev, libcupsimage2-dev" )
set ( CPACK_DEBIAN_PACKAGE_SECTION "libs" )
set ( CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/klirichek/zj-58")

if ( APPLE )
	set ( FILTERPATH "usr/libexec/cups/filter" )
	set ( PPDPATH "Library/Printers/PPDs/Contents/Resources")
	set ( OWNER "root:wheel")
	#install ( CODE "EXECUTE_PROCESS(COMMAND /etc/init.d/cups stop)" )
	install ( TARGETS rastertozj DESTINATION ${FILTERPATH} )
	# this line sets correct target permissions, due to CUPS requirements
	# However as a side effect you'll need either fakeroot, either sudo to even perform 'make package' because of it.
	install ( CODE "EXECUTE_PROCESS(COMMAND chown ${OWNER} \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${FILTERPATH}/rastertozj\")" )
	install ( FILES ${CMAKE_CURRENT_BINARY_DIR}/zj58.ppd DESTINATION "${PPDPATH}" )
	#	install ( CODE "EXECUTE_PROCESS(COMMAND /etc/init.d/cups start)" )
elseif ( UNIX )
	set ( FILTERPATH "usr/lib/cups/filter" )
	set ( PPDPATH "usr/share/cups/model/zjiang/" )
	set ( OWNER "root:root")
	install ( CODE "EXECUTE_PROCESS(COMMAND /etc/init.d/cups stop)" )
	install ( TARGETS rastertozj DESTINATION ${FILTERPATH} )
	# this line sets correct target permissions, due to CUPS requirements
	# However as a side effect you'll need either fakeroot, either sudo to even perform 'make package' because of it.
	install ( CODE "EXECUTE_PROCESS(COMMAND chown ${OWNER} \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${FILTERPATH}/rastertozj\")" )
	install ( FILES ${CMAKE_CURRENT_BINARY_DIR}/zj58.ppd DESTINATION "${PPDPATH}" )
	install ( CODE "EXECUTE_PROCESS(COMMAND /etc/init.d/cups start)" )
endif()

include (CPack)

