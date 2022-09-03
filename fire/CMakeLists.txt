add_executable(fire
        fire
        )

# pull in common dependencies
target_link_libraries(fire pi pico_stdlib hardware_gpio hardware_pio)

# create map/bin/hex file etc.
pico_add_extra_outputs(fire)

# enable usb output, disable uart output
pico_enable_stdio_usb(fire 1)
pico_enable_stdio_uart(fire 0)
