cmake_minimum_required(VERSION 3.24)

set(CMAKE_CXX_STANDARD 17)
project(shadersBasics LANGUAGES CXX)

# set Visual Studio startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})


# project options
option(SHADERS_BASICS_USE_PRE_BUILT_LIB "Use pre-built libraries or custom build them" OFF)

# GLFW options
option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
option(GLFW_INSTALL "Generate installation target" OFF)
option(GLFW_DOCUMENT_INTERNALS "Include internals in documentation" OFF)

# Update submodules as needed
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
	option(GIT_SUBMODULE "Check submodules during build" ON)
	if(GIT_SUBMODULE)
		message(STATUS "Submodule update")
		execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
						WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
						RESULT_VARIABLE GIT_SUBMOD_RESULT)
		if(NOT GIT_SUBMOD_RESULT EQUAL "0")
			message(FATAL_ERROR "git submodule update --init --recursive failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
		endif()
	endif()
endif()


add_subdirectory(src)
if (NOT ${SHADERS_BASICS_USE_PRE_BUILT_LIB})
	add_subdirectory(lib)
endif()


if(UNIX AND NOT APPLE)
	set(LINUX TRUE)
endif()

if(MSVC)
	target_compile_options(${PROJECT_NAME} PUBLIC "/W4;/analyze;/MP;")
elseif(GNU OR Clang)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS};-Wall;-Wextra;-Wpedantic;-Wconversion;-Wshadow;")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG};-O0;-g;")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE};-O3;")

	if(LINUX)
		target_link_libraries(${PROJECT_NAME} pthreads)
	endif()
endif()


find_package(Vulkan REQUIRED)


target_include_directories(
	${PROJECT_NAME}
	PUBLIC
	"src/"
	"lib/"
	"lib/glm/"
	"lib/glfw/include/"
	"lib/spdlog/include/"
	"lib/imgui/"
	${Vulkan_INCLUDE_DIR}
)


set(
	DEBUG_BUILD_LIB
	"${CMAKE_SOURCE_DIR}/binaries/glfw3.lib"
	"${CMAKE_SOURCE_DIR}/binaries/imgui.lib"
	"${CMAKE_SOURCE_DIR}/binaries/spdlogd.lib"
)
set(
	RELEASE_BUILD_LIB
	"${CMAKE_SOURCE_DIR}/binaries/glfw3.lib"
	"${CMAKE_SOURCE_DIR}/binaries/imgui.lib"
	"${CMAKE_SOURCE_DIR}/binaries/spdlog.lib"
)


if(CMAKE_BUILD_TYPE MATCHES Debug AND ${SHADERS_BASICS_USE_PRE_BUILT_LIB})
	set(BUILD_LIB ${DEBUG_BUILD_LIB})
elseif(CMAKE_BUILD_TYPE MATCHES Release AND ${SHADERS_BASICS_USE_PRE_BUILT_LIB})
	set(BUILD_LIB ${RELEASE_BUILD_LIB})
else()
	set(
		BUILD_LIB

		glfw
		spdlog::spdlog
		imgui
	)
endif()

target_link_libraries(
	${PROJECT_NAME}
	${BUILD_LIB}
	${Vulkan_LIBRARY}
)


# compile shaders
set(SHADER_SRC "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders")
set(SHADER_BIN "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/out")

file(
	GLOB SHADERS
	"${SHADER_SRC}/*.vert"
	"${SHADER_SRC}/*.frag"
)

add_custom_command(
	COMMAND
	"${CMAKE_COMMAND}" -E make_directory "${SHADER_BIN}"
	OUTPUT "${SHADER_BIN}"
	COMMENT "Creating ${SHADER_BIN}"
)

foreach(source IN LISTS SHADERS)
	get_filename_component(FILENAME "${source}" NAME)
	add_custom_command(
		COMMAND
		"${Vulkan_GLSLC_EXECUTABLE}" "${source}" -o "${SHADER_BIN}/${FILENAME}.spv"
		OUTPUT "${SHADER_BIN}/${FILENAME}.spv"
		DEPENDS "${source}" "${SHADER_BIN}"
		COMMENT "Compiling ${FILENAME}")
	list(APPEND SPV_SHADERS "${SHADER_BIN}/${FILENAME}.spv")
endforeach()

add_custom_target(shaders ALL DEPENDS ${SPV_SHADERS})
