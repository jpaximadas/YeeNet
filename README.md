# Instructions
 1. make -C libopencm3 # (Only needed once)
 2. make -C my-project

If you have an older git, or got ahead of yourself and skipped the ```--recurse-submodules```
you can fix things by running ```git submodule update --init``` (This is only needed once)

# Directories
* my-project contains your application
* my-common-code contains something shared.

# How to upload
* st-flash write awesomesauce.bin 0x8000000 (do not autoreset the MCU--memory will be erased!)
