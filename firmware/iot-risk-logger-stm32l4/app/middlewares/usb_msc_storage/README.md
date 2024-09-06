# USB MSC Middleware
## Overview
Glue code to connect the STM32 USB MSC stack IRQ functions to NOR Flash driver.

## Debugging on MacOS
```bash
diskutil list # Find the disk number of the USB device e.g. /dev/disk2
sudo dd if=/dev/disk10 of=./usb_raw_data.bin bs=512 count=1 # Read 1 512B block to ./usb_raw_data.bin
xxd ~/usb_raw_data.bin | less # View the raw data
sudo dd if=/dev/disk10 | hexdump -C # Read the entire disk and pipe to hexdump
``` 