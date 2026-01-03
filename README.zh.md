# Slint-LovyanGFX

本库将 [Slint](https://slint.dev/) 对 [ESP32 的支持](https://components.espressif.com/components/slint/slint) 从 ESP-IDF 移植到了 **PlatformIO + Arduino** 平台。

采用 [LovyanGFX](https://github.com/lovyan03/LovyanGFX) 作为图形后端，因此可以驱动各种各样的 LCD/TFT 屏幕，包含触摸支持，甚至包括墨水屏等，支持范围远超 `esp-bsp`。

> **注意**：当前这个 Slint 的移植比较 **实验性**。已验证可以运行一些简单的界面，但实际上现在我也没法让它显示 `AboutSlint` 组件，也许是它有比较大的图片资源？
> 
> **欢迎大佬贡献代码！**
> 
> **测试环境**：PC Linux x86_64, ESP32-S3 + ST7789。理论上可以在其他平台使用。

## 使用方法

建议先参考 `examples/simple/` 里的例程。

### 1. 添加依赖与屏幕配置

LovyanGFX 是必须的。在你的 `platformio.ini` 中显式添加 `lovyan03/LovyanGFX` 作为项目依赖。

```ini
lib_deps = 
    lovyan03/LovyanGFX
```

参考 LovyanGFX 的用法，配置好你 **屏幕所对应的驱动以及相关引脚**（例如在 `include/lgfx.hpp` 中）。
在代码中调用 `gfx.init();`。

**请确保，先能使用基础的 LovyanGFX 代码成功点亮屏幕，然后再尝试集成 Slint。**

### 2. platformio.ini 配置

由于 Slint 高度依赖 Modern C++ 的许多特性，**必须** 在 `platformio.ini` 中特别添加以下编译标志，否则编译时会遇到大量 C++ 17 和 20 不兼容的错误：

```ini
; === IMPORTANT FOR Slint_LovyanGFX ===
build_unflags = 
    -std=gnu++17
build_flags = 
    -std=gnu++20
```

### 3. 头文件 include 顺序 (特别注意)

`#include "slint-lgfx.h"` **应当是第一个** include 的 header。
主要是因为先引入 `Arduino.h` 会影响一些全局定义，导致编译错误。

```cpp
#include <slint-lgfx.h> // 必须放在第一位
#include <Arduino.h>
#include "lgfx.hpp"     // 你的 LovyanGFX 配置
// ...
```

### 4. 初始化与运行

写法完全类似于 Slint 在 [ESP-IDF 上的用法](https://docs.slint.dev/latest/docs/cpp/mcu/esp_idf)。

```cpp
// 1. 初始化 LovyanGFX
gfx.init();

// 2. 初始化 Slint-LovyanGFX
slint_esp_init(SlintPlatformConfiguration{
    .size = slint::PhysicalSize({(uint32_t)gfx.width(), (uint32_t)gfx.height()}),
    .gfx = &gfx, // LovyanGFX 实例指针
    .byte_swap = true
});

// 3. 创建并运行 UI
auto ui = MainWindow::create();
ui->run();
```

### 5. FreeRTOS 任务运行

由于 Slint 一般需要较大的内存栈空间，建议在一个 **FreeRTOS task** 中运行 Slint 任务（或者配置 Arduino 默认任务使用更大的栈空间）。具体实现依旧参考 `examples/simple/`。

与 Slint 侧的交互，可直接使用 [Slint C++ bindings](https://docs.slint.dev/latest/docs/cpp/overview)，如 `slint::run_event_loop();` 等。

### 6. Slint UI 文件结构

`.slint` 文件应当被放置在项目的 `src/ui/` 目录下。编译时，它们会被自动识别并处理。
例如：

```
src/
├── main.cpp
└── ui/
    └── app-window.slint
```

你可以直接在 `main.cpp` 中 `#include "app-window.h"` (根据你的 slint 文件名生成) 来获得 Slint 实例、以及 `.slint` 所导出的组件实例（比如 `MainWindow`）等。

### 7. SDK 与 编译器安装

编译时，会尝试 **自动从 Github 下载** Slint C++ SDK 和 slint-compiler 来把 Slint 链接到 ESP32 上去。

**手动安装 (如果网络有问题):**

如果有网络问题，可以手动下载：[Slint Releases](https://github.com/slint-ui/slint/releases)

1.  **Compiler**: 需要 `tools/slint-compiler`。根据 PC 架构选择，比如 `slint-compiler-Linux-x86_64.tar.gz` 或者 `slint-compiler-Windows-AMD64.tar.gz`。
2.  **SDK**: 需要 `sdk/*`。根据 MCU 型号选择，比如 `Slint-cpp-1.14.1-xtensa-esp32s3-none-elf.tar.gz`。

把它们分别解压后放在 `.pio/libdeps/[your env]/Slint_LovyanGFX/` 下，结构示例如下：

```
.pio/libdeps/esp32-s3-devkitc-1/Slint_LovyanGFX/
├── include
│   └── ...
├── tools/
│   └── slint-compiler  (可执行文件)
├── sdk/
│   └── Slint-cpp-1.14.1-xtensa-esp32s3-none-elf/
│       ├── include/
│       └── lib/
└ ...
```

## FAQ

### 屏幕颜色显示的很奇怪？像素颗粒感重？颜色发灰/发紫？

如果发现屏幕颜色显示的很奇怪，像素颗粒感重、颜色蒙了一层灰/紫色、类似油画、纱窗质感。这可能是由于 **屏幕和 MCU 的大端小端 (Endianness) 反了**。
尝试切换 `SlintPlatformConfiguration` 里的 `byte_swap` 配置：如果当前是 `true`，就换成 `false`；如果当前是 `false`，就换成 `true`。
