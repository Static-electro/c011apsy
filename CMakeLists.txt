cmake_minimum_required( VERSION 3.8 )

if( NOT DEFINED PROJECT_NAME )
    set( BUILD_C011APSY_SAMPLE ON )
endif()

project( c011apsy )

add_library( ${PROJECT_NAME} INTERFACE )
target_compile_features( ${PROJECT_NAME} INTERFACE cxx_std_11 )
target_include_directories( ${PROJECT_NAME} INTERFACE include )

if ( BUILD_C011APSY_SAMPLE )
	add_executable( sample sample/sample.cpp )
	target_link_libraries( sample ${PROJECT_NAME} )
endif()
