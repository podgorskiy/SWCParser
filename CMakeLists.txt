project(SWCParser)
cmake_minimum_required(VERSION 2.8)

option(SWCP_BUILD_TEST "Build Test Application" ON)

set(SWCParserInclude_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(SWCP_BUILD_TEST)
	include_directories(${SWCParserInclude_DIR})
	add_executable(SWCParserTest ${CMAKE_CURRENT_SOURCE_DIR}/tests/test.cpp)
endif()

install (FILES ${SWCParserInclude_DIR}/SWCP.h DESTINATION include)
