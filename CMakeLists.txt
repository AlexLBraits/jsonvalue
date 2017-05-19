cmake_minimum_required(VERSION 3.8)

project(jsonvalue)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(.)
include_directories(./3rdparty/jsmn)
include_directories(./3rdparty/utf8)

add_subdirectory(./3rdparty/jsmn ${CMAKE_BINARY_DIR}/jsmn)
add_subdirectory(./3rdparty/utf8 ${CMAKE_BINARY_DIR}/utf8)

set(HEADERS
	./linkedmap.h
	./schema.h
	./value.h
  ./stringutils.h
	)

set(SRCS 
	./schema.cpp
	./value.cpp
  ./stringutils.cpp
	)

add_library (${PROJECT_NAME} STATIC ${SRCS} ${HEADERS})
target_link_libraries(${PROJECT_NAME} jsmn utf8)
