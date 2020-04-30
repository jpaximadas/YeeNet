> :warning: **This software defaults to transmit on the 915MHz ISM band legal in the United States. Transmitting on this band may not be legal in your country.**

# How to compile

```
 $ git clone --recurse-submodules https://github.com/jpaximadas/YeeNet.git
 $ cd YeeNet
 $ make -C libopencm3
 $ make -C my-project
```

If you have an older git, or got ahead of yourself and skipped the ```--recurse-submodules```
you can fix things by running ```git submodule update --init``` (This is only needed once)

Subsequent changes to the source files only require ```make -C my-project```

# How to upload
This repository uses the stlink open source STM32 MCU programming toolset:
https://github.com/stlink-org/stlink

 1. Ensure BOOT0 is not set such that the board will erase the memory after being programmed
 2. Ensure SWDIO, SWCLK, and GND are connected to the MCU (only connect 3.3V if you intend to power the board from the programmer)
 3. Program the chip:
  ```
  $ ./st-flash --reset write awesomesauce.bin 0x8000000
  ```

 # How to debug
Once the binary has been uploaded run the following in the the my-project directory:
 1. Start the gdb server 
 ```
 $ ./st-util
 ```
 2. In another terminal--still in my-project--start GDB and configure
 ```
 $ arm-none-eabi-gdb awesomesauce.elf
 $ target remote localhost:4242
 $ load awesomesauce.elf
```
Further GDB on STM reading:

https://www.st.com/resource/en/user_manual/dm00613038-stm32cubeide-stlink-gdb-server-stmicroelectronics.pdf

Note that this pdf uses the STMCube GDB server, not the open source stlink one. However, from the perspective of the GDB client, there isn't a difference.
Page 6/15 of the pdf shows how to use breakpoints and watchpoints.

 # How to use serial
You can use GNU screen to interact with the board over serial.
Note this program does not provide echo so your text won't appear on the screen as you type. The blinking indicators on whatever USB to UART device you are using will indicate if the command works.
```
 $ screen [PORT] 115200
```
```[PORT]``` should be replaced with the USB port the USB to UART chip is using.

Example:

```
 $ screen /dev/ttyUSB0 115200
```

# Directories
* my-project contains the program
* my-common-code contains shared files from libopencm3

# Breadboard setup

Development for this project currently uses a bluepill dev board. The following table shows how the pins are connected to the SX127x and USB to UART. Please read the warnings at the end of the section before attempting to the breadboard this.

| Function | Bluepill Pin |
|---|---|
| Serial TX | A9 |
| Serial RX | A10 |
| IRQ | A0 |
| MOSI | PA7 |
| MISO | PA8 |
| SCK | PA9 |
| CS/SS | A1 |
| RST | B9 |

> :warning: **Do not power the bluepill from more than one voltage source.** This will damage the regulator on the PCB.

> :warning: **Power the SX127x or SX127x dev board with 3.3 volts ONLY.** The layout of the pins above routes signals from the SX127x into pins of the bluepill that are NOT 5 volt tolerant. The Adafruit SX127x breakout will emit 5 volt logic signals and damage the bluepill if powered from 5 volts.
## Recommeded way to power the boards
1. Power the bluepill from 5 volts from the USB to UART. The bluepill will regulate this down to 3.3 volts for its own use.
2. Power the SX127x from a decent 3.3 volt regulator. The bluepill's 3.3 volt regulator is not good enough for the task. Use the one on your USB to UART if it's present.
3. Leave 3.3 volts on the ST-Link programmer disconnected.

# Notes
## SX127x Notes
* The reset pin on the adafruit breakout board must be held LOW in order for the device to function, contrary to the instructions on the adafruit website.
* The reset pin must be pulled low momentarily in order to reset the device when the mcu reboots. This guarantees registers are reset to the original state. Failing to do this can cause weird behavior.
* In order to clear the IRQ flags register, zeros must be written twice over SPI. This is  a hardware bug.
* When putting the SX127x into LoRa mode, it must be put into the SLEEP mode first. Not STANDBY or any other modes.
* The SPI master may not write to the FIFO outside of STANDBY mode
* The FIFO is not always cleared during a mode change. Never assume the FIFO is automatically cleared
* Explicit header mode in SF=6 does not work. The automatic modulation config function in this software will not reject that modulation config.
* This code does not touch the "sync word" byte that lets LoRas reject packets automatically.
## Bluepill Dev Board Notes
* The PC13 LED on the "bluepill" dev board has it's ANODE tied to 3.3 volts and it's CATHODE tied to PC13 (for some reason). This means PC13 must be pulled low to turn on the LED.
* Do not attempt to power the SX127x from the bluepill's 3.3 volt supply. The on-board regulator cannot accomplish the task during RX or TX. Your SX127x will brown out and reset.
## USB to UART Notes
* Using an Arduino board with the CH430 USB to UART chip as your USB to UART device,is not a simple task. What is labelled as TX on the PCB (but is actually RX from the Arduino's perspective) can be put into a 5 volt tolerant TX pin of your dev board. Here comes the hard part. What is laballed as RX on the PCB (actually TX from the Arduino's perspective) won't work if you just plug it into the RX pin of your dev board. You need to solder directly to the CH430 chip. Consult the datasheet to find where the CH430's TX pin is. The letter at the end of "CH430" matters a great deal.

# Yee: https://youtu.be/IEPv31_E__4
![yee](yee.jpg)
