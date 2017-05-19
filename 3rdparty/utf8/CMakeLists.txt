cmake_minimum_required(VERSION 3.8)

project(utf8)

include_directories(.)

set(HEADERS
	./utf8.h
	)

set(SRCS 
	./utf8.c
	)

add_library (${PROJECT_NAME} STATIC ${SRCS} ${HEADERS})
