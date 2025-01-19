cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_EXTENSIONS ON)
#set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PICO_BOARD pico2_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(PicoRC C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(PicoRC
		defines/config.h
		main.c
)

pico_set_program_name(PicoRC "PicoRC")
pico_set_program_version(PicoRC "0.1")

get_target_property(PICO_STD_INCLUDE pico_stdlib INTERFACE_INCLUDE_DIRECTORIES)

target_compile_options(PicoRC PRIVATE
		$<$<CONFIG:Debug>:
		-Og
		-g
		#-Wall
		#-Wextra
		#-Winline
		#-Werror
		-Wno-error=old-style-declaration
		-march=armv8-m.main
		-mtune=cortex-m33
		-mfpu=fpv5-sp-d16
		-mthumb
		>

		$<$<CONFIG:Release>:
		-Wall
		-Wextra
		-Winline
		#-Werror
		-Wno-error=old-style-declaration
		-O1
		-s
		-g0
		-march=armv8-m.main
		-mtune=cortex-m33
		-mfpu=fpv5-sp-d16
		-mthumb
		>
)

target_compile_options(PicoRC PRIVATE
		$<$<COMPILE_LANGUAGE:C>:-Wno-old-style-declaration>
)

target_compile_definitions(PicoRC PRIVATE
		CYW43_PIO_CLOCK_DIV_DYNAMIC=1
)

target_link_options(PicoRC PRIVATE
		-march=armv8-m.main
		-mtune=cortex-m33
		-mfpu=fpv5-sp-d16
		-mthumb
)

# Generate PIO headers
#pico_generate_pio_header(PicoRC ${CMAKE_CURRENT_LIST_DIR}/modules/numbers/pio_numbers.pio)

# Modify the below lines to enable/disable output over UART/USB
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	pico_enable_stdio_uart(PicoRC 1)
	pico_enable_stdio_usb(PicoRC 0)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
	pico_enable_stdio_uart(PicoRC 0)
	pico_enable_stdio_usb(PicoRC 1)
else()
	message(WARNING "Unhandled build type: ${CMAKE_BUILD_TYPE}")
endif()

# Add the standard library to the build
target_link_libraries(PicoRC
		pico_stdlib)

# Add the standard include files to the build
target_include_directories(PicoRC PRIVATE
		${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(PicoRC
#		hardware_i2c
		pico_cyw43_arch_none
		hardware_clocks
		hardware_pio
#		hardware_adc
#		hardware_dma
#		hardware_pwm
#		pico_rand
		$<$<CONFIG:Debug>:hardware_uart>
)

pico_add_extra_outputs(PicoRC)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
	add_custom_command(
			TARGET PicoRC
			POST_BUILD
			COMMAND /usr/bin/picotool load -f PicoRC.uf2
	)
endif ()
