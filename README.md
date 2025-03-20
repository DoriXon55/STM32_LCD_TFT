# STM32 LCD TFT Display Driver Project

## Overview
This project implements a sophisticated driver for TFT LCD displays using STM32 microcontrollers. It features a robust communication protocol, efficient graphics rendering, and support for various display operations including text rendering, shape drawing, and image display.

## Key Features
- **Hardware Interface Support**
  - SPI Communication with display
  - DMA transfers for efficient data handling
  - Hardware abstraction layer for easy portability
  - Support for ST7735S display controller

- **Graphics Capabilities**
  - Text rendering with multiple font sizes (5x7, 5x8, 6x9)
  - Basic shape drawing (rectangles, triangles)
  - Pixel-level control
  - Frame buffer management
  - Scrolling text support
  - Color support (16-bit RGB565 format)

- **Display Specifications**
  - Resolution: 160x128 pixels
  - 16-bit color depth
  - Hardware-specific offset handling (LCD_OFFSET_X: 1, LCD_OFFSET_Y: 2)

## Hardware Requirements
- STM32L4 series microcontroller (or compatible)
- TFT LCD Display with ST7735S controller
- Connections:
  - SPI interface (SPI2)
  - GPIO pins for:
    - CS (Chip Select)
    - DC (Data/Command)
    - RST (Reset)
    - BL (Backlight)

## Software Architecture

### Core Components
1. **LCD Driver (lcd.c/h)**
   - Basic display initialization
   - Low-level communication
   - Frame buffer management
   - Pixel operations

2. **Graphics Library (hagl)**
   - Hardware abstraction layer
   - Drawing primitives
   - Text rendering
   - Color management

3. **Frame Handler (frame.c)**
   - Command processing
   - Animation support
   - Text scrolling management

### Communication Protocol
The project implements a robust frame-based communication protocol with:
- Frame start/end markers
- Byte stuffing for reliable data transmission
- CRC16 error checking
- Command-based operation structure

## Supported Commands
- `ONK` - Basic shape drawing operations
- `ONP` - Point/pixel manipulation
- `ONT` - Triangle drawing (filled and outline)
- `ONN` - Text display with scrolling support
- `OFF` - Display control (backlight, clear screen)

## Building and Flashing
1. Clone the repository:
```bash
git clone https://github.com/DoriXon55/STM32_LCD_TFT.git
```

2. Build using your preferred STM32 development environment:
   - STM32CubeIDE
   - Make (Makefile provided)



## Technical Highlights
- Uses DMA for efficient display updates
- Handles display offsets automatically
- Supports multiple font sizes
- Efficient memory usage with 16-bit color format

## Development Tools Used
- STM32CubeIDE
- STM32 HAL Libraries
- Custom HAGL (Hardware Abstraction Graphics Library) implementation


## Author
- [DoriXon55](https://github.com/DoriXon55)

## Acknowledgments
- STM32 Community
- HAL Library developers
- HAGL graphics library inspiration
  
[MIT](https://opensource.org/license/mit)
