set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(TOOLCHAIN_PREFIX aarch64-none-elf-)

if(DEFINED ENV{DEVKITPRO})
	set(DEVKITPRO "$ENV{DEVKITPRO}")
else()
	set(DEVKITPRO "/opt/devkitpro")
endif()

if(DEFINED ENV{DEVKITA64})
	set(DEVKITA64 "$ENV{DEVKITA64}")
else()
	set(DEVKITA64 "${DEVKITPRO}/devkitA64")
endif()

# Without that flag CMake is not able to pass test compilation check
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_COMPILER "${DEVKITA64}/bin/${TOOLCHAIN_PREFIX}gcc")
set(CMAKE_ASM_COMPILER "${CMAKE_C_COMPILER}")
set(CMAKE_CXX_COMPILER "${DEVKITA64}/bin/${TOOLCHAIN_PREFIX}g++")

set(CMAKE_LD "${DEVKITA64}/bin/${TOOLCHAIN_PREFIX}ld" CACHE INTERNAL "linker tool")
set(CMAKE_OBJCOPY "${DEVKITA64}/bin/${TOOLCHAIN_PREFIX}objcopy" CACHE INTERNAL "objcopy tool")
set(CMAKE_SIZE_UTIL "${DEVKITA64}/bin/${TOOLCHAIN_PREFIX}size" CACHE INTERNAL "size tool")

set(CMAKE_FIND_ROOT_PATH "${DEVKITA64}/bin")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# devkitA64 flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -MMD -MP") # -MF

# nx flags
add_definitions(-D__SWITCH__)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv8-a -mtune=cortex-a57 -mtp=soft -fPIE")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}") # -fno-exceptions -fno-rtti

# libnx
include_directories("$ENV{DEVKITPRO}/libnx/include")
link_directories("$ENV{DEVKITPRO}/libnx/lib")

# portlibs
include_directories("$ENV{DEVKITPRO}/portlibs/switch/include")

function(add_nx_target)
	set(_SINGLE_VALUE_ARGS NAME)
	set(_MULTI_VALUE_ARGS SOURCES LIBRARIES)
	cmake_parse_arguments(_NX_TARGET "" "${_SINGLE_VALUE_ARGS}" "${_MULTI_VALUE_ARGS}" ${ARGN})

	add_executable("${_NX_TARGET_NAME}.elf" ${_NX_TARGET_SOURCES})
	target_link_libraries("${_NX_TARGET_NAME}.elf" nx ${_NX_TARGET_LIBRARIES})
	set_target_properties("${_NX_TARGET_NAME}.elf" PROPERTIES
			#COMPILE_FLAGS "-MF ${_NX_TARGET_NAME}.d"
			LINK_FLAGS "-specs=${DEVKITPRO}/libnx/switch.specs -Wl,-no-as-needed -Wl,-Map,${_NX_TARGET_NAME}.map")

	add_custom_target("${_NX_TARGET_NAME}.nso" ALL
			BYPRODUCTS "${_NX_TARGET_NAME}.nso"
			COMMAND "${DEVKITPRO}/tools/bin/elf2nso"
			"${CMAKE_CURRENT_BINARY_DIR}/${_NX_TARGET_NAME}.elf"
			"${CMAKE_CURRENT_BINARY_DIR}/${_NX_TARGET_NAME}.nso"
			COMMENT "Generating NSO")
	add_dependencies("${_NX_TARGET_NAME}.nso" "${_NX_TARGET_NAME}.elf")

	add_custom_target("${_NX_TARGET_NAME}.nro" ALL
			BYPRODUCTS "${_NX_TARGET_NAME}.nro"
			COMMAND "${DEVKITPRO}/tools/bin/elf2nro"
			"${CMAKE_CURRENT_BINARY_DIR}/${_NX_TARGET_NAME}.elf"
			"${CMAKE_CURRENT_BINARY_DIR}/${_NX_TARGET_NAME}.nro"
			COMMENT "Generating NRO")
	add_dependencies("${_NX_TARGET_NAME}.nro" "${_NX_TARGET_NAME}.elf")
endfunction()