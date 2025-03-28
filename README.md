## Space Invaders on STM32 with OLED SSD1339



This project implements the classic Space Invaders game on an STM32 microcontroller, tested on the NUCLEO-L476RG board. The game uses an OLED SSD1339 display and allows control via a computer using the UART protocol (commands can be sent via PuTTY, for example).

![1742632955845.jpg](https://github.com/user-attachments/assets/12b8e970-c9ec-41cb-b017-6bf1b40d21d6)

---

### Project Features

Game display on OLED SSD1339 (SPI communication)

User input via UART (e.g., a;d keys for movement, spacebar for shooting)

Basic game mechanics: enemies, bullets, collisions

Optimized for STM32 microcontroller performance


### Hardware Requirements

NUCLEO-L476RG (STM32L476RG)

OLED SSD1339 connected via SPI

Computer connection via UART (e.g., USB-UART adapter or built-in ST-Link Virtual COM Port)


### Setup and Execution

1. Configure the hardware: connect the OLED SSD1339 display to the SPI interface of the microcontroller.


2. Configure UART in STM32CubeIDE with a baud rate of 115200.


3. Flash the code to the microcontroller.


4. Connect to the UART port using PuTTY or another terminal and start controlling the game.


