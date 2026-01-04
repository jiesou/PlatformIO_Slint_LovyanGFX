# Slint-LovyanGFX

[简体中文](./README.zh.md)

This library ports [Slint](https://slint.dev/)'s [ESP32 support](https://components.espressif.com/components/slint/slint) from ESP-IDF to the **PlatformIO + Arduino** environment.

It uses [LovyanGFX](https://github.com/lovyan03/LovyanGFX) as the graphics backend, enabling support for a wide variety of LCD/TFT screens, touchscreens, and even e-ink displays (far beyond the limited support in `esp-bsp`).

> **Warning**: This port is currently **experimental**. It has been validated with simple interfaces. Complex interfaces (e.g., `AboutSlint` with heavy image resources) may fail. Contributions are welcome!

> **Tested On**: PC Linux x86_64, ESP32-S3 + ST7789. Should compatible with other platforms.

https://github.com/user-attachments/assets/62571ef6-72a0-482f-9a48-f4d0882f484e

## How to Use

Recommended to follow the `examples/simple/` example project for a quick setup.

### 1. Dependencies & Setup

LovyanGFX is needed. Add `lovyan03/LovyanGFX` explicitly to your `lib_deps` in `platformio.ini`.

```ini
lib_deps = 
    lovyan03/LovyanGFX
```

You need to configure your screen driver and pins using LovyanGFX. Typically, you would create a setup file (e.g., `include/lgfx.hpp`) and call `gfx.init()` in your setup code.

**Ensure: your screen works correctly with basic LovyanGFX examples, before attempting to use Slint.**

### 2. PlatformIO Configuration

Slint relies heavily on Modern C++ features. You **must** configure `platformio.ini` to use C++20 and unflag C++17 (or lower) to avoid compilation errors.

```ini
; === IMPORTANT FOR Slint_LovyanGFX ===
build_unflags = 
    -std=gnu++17
build_flags = 
    -std=gnu++20
```

### 3. Include Order (Important)

In your main file (e.g., `main.cpp`), you **must** include `slint-lgfx.h` before any other header, especially before `Arduino.h`.

```cpp
#include <slint-lgfx.h> // Must be first
#include <Arduino.h>
#include "lgfx.hpp"     // Your LovyanGFX configuration
// ...
```

### 4. Initialization

The initialization is similar to the [Slint ESP-IDF documentation](https://docs.slint.dev/latest/docs/cpp/mcu/esp_idf).

```cpp
// Initialize LovyanGFX first
gfx.init();

// Initialize Slint-LovyanGFX
slint_esp_init(SlintPlatformConfiguration{
    .size = slint::PhysicalSize({(uint32_t)gfx.width(), (uint32_t)gfx.height()}),
    .gfx = &gfx, // Pointer to your LovyanGFX instance
    .byte_swap = true
});

// Create and run the UI
auto ui = MainWindow::create();
ui->run();
```

### 5. Task Configuration

Slint typically requires a larger stack size than the default Arduino loop provides. It is highly recommended to run the Slint event loop in a separate **FreeRTOS task** with an increased stack size. Refer to `examples/simple/`.

For interaction with the UI, use the standard [Slint C++ bindings](https://docs.slint.dev/latest/docs/cpp/overview).

### 6. Slint UI Files

Your `.slint` files should be placed in the `src/ui/` directory. They will be automatically recognized and compiled.
For example:

```
src/
├── main.cpp
└── ui/
    └── app-window.slint
```

In your `main.cpp`, you can directly include the generated header (named after your slint file) to access the Slint instance and components (e.g., `MainWindow`).

```cpp
#include "app-window.h" // Generated from app-window.slint
```

### 7. SDK & Compiler Installation

During compilation will attempt to automatically download the Slint C++ SDK and slint-compiler from GitHub to link Slint to your ESP32.

**Manual Installation (if network fails):**

You can manually download the required files from [Slint Releases](https://github.com/slint-ui/slint/releases).

1.  **Compiler**: Download `slint-compiler` for your PC architecture (e.g., `slint-compiler-Linux-x86_64.tar.gz` or `slint-compiler-Windows-AMD64.tar.gz`).
2.  **SDK**: Download the SDK for your MCU (e.g., `Slint-cpp-1.14.1-xtensa-esp32s3-none-elf.tar.gz`).

Extract them into `.pio/libdeps/[your env]/Slint_LovyanGFX/` like:

```
.pio/libdeps/esp32-s3-devkitc-1/Slint_LovyanGFX/
├── include
│   └── ...
├── tools/
│   └── slint-compiler  (Executable)
├── sdk/
│   └── Slint-cpp-1.14.1-xtensa-esp32s3-none-elf/
│       ├── include/
│       └── lib/
└ ...
```

## FAQ

### Screen colors look weird / washed out / pixelated?

If your screen colors look strange, have a heavy pixelated feel, are covered in a gray/purple haze, or look like an oil painting or screen door effect, this is likely due to a **Little-Endian / Big-Endian mismatch** between the screen and the MCU.

Try toggling the `byte_swap` setting in `SlintPlatformConfiguration`. If it is `true`, change it to `false`, if it is `false`, change it to `true`.
