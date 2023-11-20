add_executable(ir_line
ir_line.c
        )

target_link_libraries(ir_line pico_stdlib hardware_adc)

# create map/bin/hex file etc.
pico_add_extra_outputs(ir_line)

# add url via pico_set_program_url
example_auto_set_url(ir_line)

pico_enable_stdio_usb(ir_line 1)
