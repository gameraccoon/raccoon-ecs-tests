cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(APP_NAME RaccoonEcsTests)
project(${APP_NAME})

set(EXECUTABLE_NAME ${APP_NAME})

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# constants
set(TESTS_BASE_DIR ${CMAKE_SOURCE_DIR})
set(ECS_INCLUDE_DIR "${TESTS_BASE_DIR}/../include")
set(BIN_PLATFORM "x64")

add_definitions(-DRACCOON_ECS_DEBUG_CHECKS_ENABLED)
add_definitions(-DRACCOON_ECS_COPYABLE_COMPONENTS)

# compiler parameters
if (MSVC)
	if (CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
		string(REGEX REPLACE "/W[0-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	endif()
	set(PROJECT_CXX_FLAGS /W4 /std:c++latest /wd4996)

	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG -DDEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}")
else()
	set(PROJECT_CXX_FLAGS -std=c++2a -Wall -Wextra -pedantic -Werror)

	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG} -g")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE} -O2")
endif()

# Executable
set(UNITTESTS_NAME ${APP_NAME})
file(GLOB UNITTESTS_SRC RELATIVE "" FOLLOW_SYMLINKS "${TESTS_BASE_DIR}/tests/*.cpp")
set(UNITTESTS_SRC
	${UNITTESTS_SRC}
	${TESTS_BASE_DIR}/third-party/googletest/src/gtest-all.cc
	${TESTS_BASE_DIR}/main.cpp
)

include_directories(
	${ECS_INCLUDE_DIR}
	${TESTS_BASE_DIR}
	${TESTS_BASE_DIR}/third-party/googletest
)
add_executable(${APP_NAME} ${UNITTESTS_SRC})
target_compile_options(${APP_NAME} PRIVATE ${PROJECT_CXX_FLAGS})

if (NOT MSVC)
	target_link_libraries(${APP_NAME}
		pthread
	)
endif()
