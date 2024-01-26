cmake_minimum_required(VERSION 3.12)

if(APPLE)
	set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
endif(APPLE)

project("rext")

if(NOT CMAKE_BUILD_TYPE)
	SET(CMAKE_BUILD_TYPE "Debug")
endif()

set(CMAKE_CXX_STANDARD 17)

FILE(GLOB SRC 
	${CMAKE_SOURCE_DIR}/*.cpp
	${CMAKE_SOURCE_DIR}/*.h	
	)

LIST(SORT SRC)

add_library(${CMAKE_PROJECT_NAME} SHARED ${SRC})

if(WIN32)
	set(PLAT "win")
elseif(APPLE)
	set(PLAT "mac")
else()
	set(PLAT "lin")
endif()

set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELEASE} -DDEBUG -O0")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -Og")

execute_process(COMMAND git rev-parse --short HEAD
	OUTPUT_VARIABLE BUILD_VERSION WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

if(${BUILD_VERSION} STREQUAL "") 
	set(BUILD_VERSION "LOCAL")
else()
	string(REGEX REPLACE "\n$" "" BUILD_VERSION "${BUILD_VERSION}")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBUILD_VERSION=\"${BUILD_VERSION}\"")
#-DBUILD_VERSION=\"\"${BUILD_VERSION}\"\"

set(XPSDK "${CMAKE_SOURCE_DIR}/SDK") 

add_definitions(-DXPLM200=1 -DXPLM210=1 -DXPLM300=1)


if(WIN32) 
	add_definitions(-DAPL=0 -DIBM=1 -DLIN=0)
elseif(APPLE)
	add_definitions(-DAPL=1 -DIBM=0 -DLIN=0)
else()
	add_definitions(-DAPL=0 -DIBM=0 -DLIN=1)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC -fvisibility=hidden --std=c++17 -fpermissive")

include_directories(
    "${XPSDK}/CHeaders/XPLM"
    "${XPSDK}/CHeaders/Widgets"
    "${XPSDK}/CHeaders/Wrappers"
)

# linking

if(WIN32)
	find_library(XPLM_LIBRARY XPLM_64 "${XPSDK}/Libraries/Win")
elseif(APPLE)
	find_library(XPLM_LIBRARY XPLM "${XPSDK}/Libraries/Mac")
else()
	set(XPLM_LIBRARY "")
endif()

target_link_libraries(${CMAKE_PROJECT_NAME}
		${XPLM_LIBRARY}
    )

if(UNIX AND NOT APPLE)
	set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES LINK_FLAGS
		"${CMAKE_SHARED_LINKER_FLAGS} -m64 -static-libgcc -static-libstdc++ -shared")
elseif(APPLE)
	#set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES LINK_FLAGS
	#	"${CMAKE_SHARED_LINKER_FLAGS} ")
elseif(WIN32 AND NOT MSVC)
	set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES LINK_FLAGS
		"${CMAKE_SHARED_LINKER_FLAGS} -m64 -static-libgcc -static-libstdc++ -shared")	
else()

endif()

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES PREFIX "")
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES SUFFIX "")
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY
	"${CMAKE_SOURCE_DIR}/Deploy/${CMAKE_PROJECT_NAME}/64/$<0:>" )
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY
	"${CMAKE_SOURCE_DIR}/Deploy/${CMAKE_PROJECT_NAME}/64/$<0:>" )
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PLAT}.xpl")

if(HOMEBREW AND UNIX)
	add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
		COMMAND cp "${CMAKE_SOURCE_DIR}/Deploy/${CMAKE_PROJECT_NAME}/64/${PLAT}.xpl"
	)
endif()