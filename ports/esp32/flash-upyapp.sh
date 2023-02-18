#!/bin/bash

# esptool.py --chip esp32c3 --port /dev/ttyUSB0 --baud 

esptool.py esp32c3 -p /dev/ttyUSB0 -b 460800 --before=default_reset --after=hard_reset write_flash --flash_mode dio --flash_freq 80m --flash_size 2MB 0x10000 micropython.bin
