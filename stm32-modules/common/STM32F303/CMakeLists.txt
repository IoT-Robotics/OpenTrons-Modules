# Sets up a linkable common library for STM32G491 targets

find_package(GDBSVDTools)

# Fills in the template with values specified by the find_package(OpenOCD) call above
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/gdbinit.template ./gdbinit)

add_library(${TARGET_MODULE_NAME}-STM32F303 STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/startup_stm32f303xe.s)

target_link_libraries(${TARGET_MODULE_NAME}-STM32F303
    PUBLIC ${TARGET_MODULE_NAME}-core)

target_link_options(${TARGET_MODULE_NAME}-STM32F303
    INTERFACE
    "LINKER:-T,${CMAKE_CURRENT_SOURCE_DIR}/STM32F303RETx_FLASH.ld"
    "LINKER:--print-memory-usage"
    "LINKER:--error-unresolved-symbols"
    "LINKER:--gc-sections"
    "LINKER:-u,_printf_float"
    "LINKER:-u,_scanf_float")

# Incurs at least a relink when you change the linker file (and a recompile of main
# but hopefully that's quick)
set_source_files_properties(./startup_stm32f303xe.s
  PROPERTIES
  OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/STM32F303RETx_FLASH.ld)
