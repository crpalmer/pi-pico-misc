add_executable(cnc-touch-probe
        cnc-touch-probe.cpp
        )

# pull in common dependencies
target_link_libraries(cnc-touch-probe pi pico_stdlib hardware_gpio)

# create map/bin/hex file etc.
pico_add_extra_outputs(cnc-touch-probe)

# enable usb output, disable uart output
pico_enable_stdio_usb(cnc-touch-probe 1)
pico_enable_stdio_uart(cnc-touch-probe 0)
