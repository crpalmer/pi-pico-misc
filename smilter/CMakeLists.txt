add_executable(smilter
        smilter.cpp
        )

# pull in common dependencies
target_link_libraries(smilter
    pi
    pico_stdlib
    pico_multicore
    hardware_adc
    hardware_gpio
    hardware_pio
)

# create map/bin/hex file etc.
pico_add_extra_outputs(smilter)

# enable usb output, disable uart output
pico_enable_stdio_usb(smilter 1)
pico_enable_stdio_uart(smilter 0)
