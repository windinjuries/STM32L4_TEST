# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build System

This project uses Keil MDK-ARM (µVision) as the primary IDE. Build using:
- Open `MDK-ARM/TEST.uvprojx` in Keil MDK-ARM
- Build with F7 or Project → Build Target

## Project Structure

- **Core/**: Application code (main.c, initialization)
- **Drivers/**: STM32 HAL and CMSIS libraries
- **Middlewares/**: Third-party libraries:
  - **FreeRTOS**: Configured in `.ioc` file
  - **LVGL**: GUI library (v8.3) in `Middlewares/Third_Party/LVGL`
  - **STM32_USB_Device_Library**: USB device implementation

## MCU Configuration

- Target: STM32L475VET6 (LQFP100 package)
- Clock configuration: 80 MHz SYSCLK (PLL from HSE 8 MHz)
- Peripherals configured via `TEST.ioc` (STM32CubeMX):
  - USART1/USART2 for serial communication
  - SPI3 for display interface
  - TIM2/TIM3 for PWM/timing
  - USB_OTG_FS for device communication
  - FreeRTOS with default task

## Development Workflow

1. Modify `TEST.ioc` with STM32CubeMX for hardware configuration changes
2. Generate code (replaces `Core/Src/main.c`, `Core/Inc/stm32l4xx_hal_conf.h`, etc.)
3. Implement application logic in `Core/Src/`
4. Build in Keil MDK-ARM

## Key Components

- **FreeRTOS**: Configured with single default task
- **LVGL**: Integrated with display driver in `Middlewares/Third_Party/LVGL`
- **USB CDC**: Configured for virtual com port
- **DMA**: Used for USART2_RX (circular mode, high priority)

## Keypad Input (4-Button Navigation)

| Button  | Pin   | Active  | Pull     | LVGL Key       | Function        |
|---------|-------|---------|----------|----------------|-----------------|
| WAKE_UP | PC13  | High    | down     | `LV_KEY_UP`    | 上 / Up         |
| KEY2    | PD8   | Low     | up       | `LV_KEY_LEFT`  | 左 / Left       |
| KEY1    | PD9   | Low     | up       | `LV_KEY_DOWN`  | 下 / Down       |
|         |       | Long>500ms|          | `LV_KEY_ENTER`| 选中 / Select   |
| KEY0    | PD10  | Low     | up       | `LV_KEY_RIGHT` | 右 / Right      |

- WAKE_UP: active-HIGH with internal pull-down (standard STM32 WKUP button)
- KEY0/KEY1/KEY2: active-LOW with internal pull-up (standard dev-board buttons)
- Driver: `Middlewares/Third_Party/LVGL/port/lv_port_indev.c`
- Long-press detection on KEY1 is handled in the indev read callback (500ms threshold)
- Group navigation is set up per-screen in `lv_app.c` (`group_home`, `group_time`, etc.)

## GUI Pages

- **Home**: Menu with 3 items (Time / Weather / System Info)
- **Time**: RTC date + time + weekday, refresh every 1s
- **Weather**: wttr.in temperature, manual refresh button, auto-refresh every 10min
- **System Info**: MCU, RTOS, LVGL, display info
- GUI code: `Middlewares/Third_Party/LVGL/app/lv_app.c`