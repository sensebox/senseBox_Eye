# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

# set(EXTRA_COMPONENT_DIRS components/esp32-arduino-lib-builder/components/arduino_tinyusb)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# Adds necessary definitions for compiling it using Serial symbol attached to the HW USB CDC port
list(APPEND compile_definitions "ARDUINO_USB_CDC_ON_BOOT=1")
list(APPEND compile_definitions "ARDUINO_USB_MODE=1")

idf_build_set_property(MINIMAL_BUILD ON)
project(arduino-as-a-component)
