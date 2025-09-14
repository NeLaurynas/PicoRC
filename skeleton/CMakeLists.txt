cmake_minimum_required(VERSION 4.00)

set(PROJECT_NAME "Skeleton")

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PICO_BOARD pico2_w CACHE STRING "Board type")

set(BLUEPAD32_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../lib/bluepad32)
set(BTSTACK_ROOT ${PICO_SDK_PATH}/lib/btstack)

add_compile_definitions(PICO_FLASH_ASSERT_ON_UNSAFE=0)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(${PROJECT_NAME} C CXX ASM)

pico_sdk_init()

add_executable(${PROJECT_NAME}
		defines/config.h
		main.c
		rc_platform.c
		shared_config.h
		state.c
		state.h
		renderer.c
		renderer.h
		modules/engine/turret_rotation.c
		modules/engine/turret_rotation.h
		modules/engine/main_engine.c
		modules/engine/main_engine.h
)

target_include_directories(${PROJECT_NAME} PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}
		${BLUEPAD32_ROOT}/src/components/bluepad32/include
)

# minimp3
#include_directories(${CMAKE_SOURCE_DIR}/../lib/minimp3)

# Create a private copy of the library inside the build tree and replace
set(PICO_SHARED_COPY_PARENT ${CMAKE_BINARY_DIR}/pico-shared_src)
file(COPY ${CMAKE_CURRENT_LIST_DIR}/../lib/pico-shared DESTINATION ${PICO_SHARED_COPY_PARENT})
set(PICO_SHARED_SRC_COPY ${PICO_SHARED_COPY_PARENT}/pico-shared)

configure_file(
		${CMAKE_CURRENT_SOURCE_DIR}/shared_config.h
		${PICO_SHARED_SRC_COPY}/shared_config.h
		COPYONLY
)

target_include_directories(${PROJECT_NAME} PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}
		${PICO_SHARED_SRC_COPY}
)

add_subdirectory(${PICO_SHARED_SRC_COPY} ${CMAKE_BINARY_DIR}/pico-shared)
target_link_libraries(${PROJECT_NAME} pico-shared)

pico_set_program_name(${PROJECT_NAME} "${PROJECT_NAME}")
pico_set_program_version(${PROJECT_NAME} "0.1")

get_target_property(PICO_STD_INCLUDE pico_stdlib INTERFACE_INCLUDE_DIRECTORIES)

target_compile_options(${PROJECT_NAME} PRIVATE
		$<$<CONFIG:Debug>:
		-Og # disable if you get <optimized out>
		-g3
		-Wall
		-Wextra
		-Winline
		#		-Werror
		-Wno-error=old-style-declaration
		-mcpu=cortex-m33
		-mtune=cortex-m33
		-mfpu=fpv5-sp-d16
		-mthumb
		-ffunction-sections
		-fdata-sections
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
		-mcpu=cortex-m33
		-mtune=cortex-m33
		-mfpu=fpv5-sp-d16
		-mthumb
		-ffunction-sections
		-fdata-sections
		>
)

target_compile_options(${PROJECT_NAME} PRIVATE
		$<$<COMPILE_LANGUAGE:C>:-Wno-old-style-declaration>
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
		CYW43_PIO_CLOCK_DIV_DYNAMIC=1
)

target_link_options(${PROJECT_NAME} PRIVATE
		-mcpu=cortex-m33
		-mtune=cortex-m33
		-mfpu=fpv5-sp-d16
		-mthumb
		-Wl,--gc-sections
)

# Generate PIO headers
#pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/modules/numbers/pio_numbers.pio)

# Modify the below lines to enable/disable output over UART/USB
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	pico_enable_stdio_uart(${PROJECT_NAME} 1)
	pico_enable_stdio_usb(${PROJECT_NAME} 0)
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
	pico_enable_stdio_uart(${PROJECT_NAME} 0)
	pico_enable_stdio_usb(${PROJECT_NAME} 1)
else ()
	message(WARNING "Unhandled build type: ${CMAKE_BUILD_TYPE}")
endif ()

include_directories(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR})
include_directories(${BTSTACK_ROOT}/3rd-party/bluedroid/encoder/include)
include_directories(${BTSTACK_ROOT}/3rd-party/bluedroid/decoder/include)
include_directories(${BTSTACK_ROOT}/src)

target_link_libraries(${PROJECT_NAME}
		pico_stdlib)

target_link_libraries(${PROJECT_NAME}
		#		hardware_i2c
		pico_cyw43_arch_none
		pico_btstack_classic
		pico_btstack_cyw43
		bluepad32
		hardware_clocks
		hardware_pio
		pico_multicore
		#		hardware_adc
		hardware_dma
		hardware_pwm
		#		pico_rand
		$<$<CONFIG:Debug>:hardware_uart>
)

add_subdirectory(${BLUEPAD32_ROOT}/src/components/bluepad32 libbluepad32)

pico_add_extra_outputs(${PROJECT_NAME})

if (CMAKE_BUILD_TYPE STREQUAL "Release")
	add_custom_command(
			TARGET ${PROJECT_NAME}
			POST_BUILD
			COMMAND /opt/homebrew/bin/picotool load -f ${PROJECT_NAME}.uf2
	)
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_custom_command(
			TARGET ${PROJECT_NAME}
			POST_BUILD
			COMMAND /usr/local/bin/openocd -s tcl -f interface/cmsis-dap.cfg -f target/rp2350.cfg -c "adapter speed 5000" -c "program ${PROJECT_NAME}.elf verify reset exit"
	)
endif ()
