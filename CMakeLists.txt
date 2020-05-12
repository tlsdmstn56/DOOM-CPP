cmake_minimum_required(VERSION 3.0.0)
project (doom-cpp)

set (CMAKE_CXX_STANDARD 14)

##############################################################
# import spdlog
set(SPDLOG_BUILD_EXAMPLES OFF)
set(SPDLOG_BUILD_TESTING OFF)
set(SPDLOG_BUILD_SHARED OFF)
get_filename_component(SPDLOG_DIR external/spdlog ABSOLUTE)
set(SPDLOG_INCLUDE_DIRS "${SPDLOG_DIR}/include")
add_subdirectory(${SPDLOG_DIR})


# find_package(spdlog CONFIG REQUIRED)


############################################################
# doom cpp
#
add_subdirectory(doom)
# add_subdirectory(sersrc)
add_subdirectory(sndserv)
# add_subdirectory(ipx)