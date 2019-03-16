cmake_minimum_required ( VERSION 3.12 )

project ( rastertozj C )

find_package ( Cups REQUIRED )
find_library ( CUPSIMAGELIB NAMES cupsimage )
mark_as_advanced ( CUPSIMAGELIB )

set ( DEBUGFILE "/tmp/debugraster.txt" CACHE STRING "File where to make debug output if DEBUGP is set" )
option ( DEBUGP "Whetner to output debug info" ON )

add_executable ( rastertozj rastertozj.c )
target_compile_options ( rastertozj PRIVATE -Wall -fPIC -O3 )
target_include_directories ( rastertozj PRIVATE ${CUPS_INCLUDE_DIR} )
target_link_libraries ( rastertozj PRIVATE ${CUPS_LIBRARIES} ${CUPSIMAGELIB} )

if ( DEBUGP )
	target_compile_definitions ( rastertozj PRIVATE DEBUGP DEBUGFILE="${DEBUGFILE}")
endif ()


