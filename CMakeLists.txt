# CMake Version 
CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

# Project Name
PROJECT(SimpleTelnetClient)

#
# Configuration
#
# SET(CMAKE_VERBOSE_MAKEFILE ON)
#

#
if ("${build}" MATCHES "debug")
 SET (CMAKE_BUILD_TYPE "debug")
else ("${build}" MATCHES "debug")
 SET (CMAKE_BUILD_TYPE "release")
endif ("${build}" MATCHES "debug")

#
# Add Compiler Option and Macro
#
ADD_DEFINITIONS(-DCALIB_CFG_BUILD_MODE="${CMAKE_BUILD_TYPE}")

# Set Build Directories
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

#
# Check OS and Processor Information 
#
SET(CMAKE_CXX_FLAGS "-std=c++11")
ADD_DEFINITIONS(-Wall -DCALIB_CFG_OS="${CMAKE_SYSTEM_NAME}" -DCALIB_CFG_CPU="${CMAKE_SYSTEM_PROCESSOR}")

# Configuration
include_directories( ./dependency/boost_1_61_0)

add_executable (SimpleTelnetClient
    src/main.cpp
    impl/AsioTelnetClient.cpp
    impl/AsioTelnetClient.h
    impl/TelnetProtocol.h
)

TARGET_LINK_LIBRARIES(SimpleTelnetClient
	pthread
)

