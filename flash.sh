#!/bin/bash

set -e

BULD_PATH="IDF_photodisplay/build"

esptool.py erase_region 0x9000 0x6000

esptool.py --chip esp32s3 -p /dev/ttyACM0 -b 460800 \
--before=default_reset --after=no_reset hard_reset --flash_mode dio --flash_freq 80m --flash_size 16MB \
0x0 ${BULD_PATH}/bootloader/bootloader.bin \
0x10000 ${BULD_PATH}/IDF_photodisplay.bin \
0x8000 ${BULD_PATH}/partition_table/partition-table.bin
