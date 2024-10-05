# USB MSC Middleware
## Overview
Glue code to connect the STM32 USB MSC stack IRQ functions to NOR Flash driver.

## Debugging on MacOS
```bash
# List all USB devices to check if the device is connected
$ ioreg -p IOUSB

# List all disks, try to find the one with the correct size e.g /dev/disk14 
$ diskutil list

# Read disk data (from e.g /dev/disk14) to file, FAT12 takes 9 512-byte blocks 
$ sudo dd if=/dev/disk14 of=./usb_raw_data.hex bs=512 count=12

$ xxd ./usb_raw_data.hex | less
``` 