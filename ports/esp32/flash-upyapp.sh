#!/bin/bash


# make VERBOSE=1  USER_C_MODULES=$PWD/usermod/dmaadc.cmake
make  USER_C_MODULES=$PWD/usermod/dmaadc.cmake

esptool.py erase_region 0x10000 0x160000

esptool.py --chip  esp32c3 -p /dev/ttyUSB0 -b 460800 --before=default_reset --after=hard_reset write_flash --flash_mode dio --flash_freq 80m --flash_size 2MB 0x10000 build-GENERIC_C3/micropython.bin
