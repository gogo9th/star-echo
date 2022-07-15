cmake_minimum_required (VERSION 3.16)

project ("q2cathedral")

set(default_build_type "Release")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()


set(Boost_USE_STATIC_LIBS ON) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 

find_package(Boost 1.71.0 REQUIRED COMPONENTS program_options)
include_directories(${Boost_INCLUDE_DIRS}) 

#find_package(ffmpeg COMPONENTS AVCODEC AVFORMAT SWRESAMPLE REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
    libavformat
    libavcodec
    libswresample
    libavutil
)

if (UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif (UNIX)

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  string(REPLACE "/Zi" "/ZI" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
endif()

add_executable(${PROJECT_NAME} "q2cathedral.cpp" "q2cathedral.h" "log.hpp"  "threaded.h"  "filter.h" "DNSE_CH.hpp"  "mediaProcess.h" "mediaProcess.cpp" "utils.h" "DNSE_EQ.hpp"  "FilterFabric.h"  "DNSE_BE.hpp" "DNSE_3D.hpp" "DbReduce.hpp" "DNSE_BE_params.cpp" "DNSE_BE_params.h" "DNSE_CH_params.cpp" "DNSE_CH_params.h")
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES} PkgConfig::LIBAV)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR})

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF
)
