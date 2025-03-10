# Instraction

To include this module to your project:
`$ git clone`

In top CMakeLists.txt:
include(path/to/rhs/rhs.cmake)

Choose your RPLC device:
```cmake
set(RPLC_XL ON) # or
set(RPLC_L ON) # or
set(RPLC_M ON)
```

You must have clone freertos-kernel and setting it:
```cmake
add_library(freertos_config INTERFACE)

target_include_directories(freertos_config SYSTEM
        INTERFACE
        ../Core/Inc # path to FreeRTOSConfig.h
)

target_compile_definitions(freertos_config
        INTERFACE
        projCOVERAGE_TEST=0
)

set(FREERTOS_HEAP "4" CACHE STRING "" FORCE)
set(FREERTOS_PORT "GCC_ARM_CM7" CACHE STRING "" FORCE)

add_subdirectory(freertos) # path to cloned rep
```
