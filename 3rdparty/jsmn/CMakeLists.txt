cmake_minimum_required(VERSION 3.8)

project(jsmn)

include_directories(.)

set(HEADERS
	./jsmn.h
	)

set(SRCS 
	./jsmn.c
	)

add_library (${PROJECT_NAME} STATIC ${SRCS} ${HEADERS})
